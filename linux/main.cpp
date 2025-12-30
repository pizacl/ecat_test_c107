// loopback_nodc.cpp
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

extern "C" {
#include <soem/soem.h>
}

static ecx_contextt ctx;
static char IOmap[4096];

static volatile int g_run = 1;
static void on_sigint(int) { g_run = 0; }

// ここでは「master outputs -> slave outputs(SM2)」が test_value_rx として送られ、
// 「slave inputs(SM3) -> master inputs」が test_value_tx として返る想定
struct TxPDO_1i32 { int32_t test_value_rx; } __attribute__((packed));
struct RxPDO_1i32 { int32_t test_value_tx; } __attribute__((packed));

static void dump_slave_states()
{
    ecx_readstate(&ctx);
    for (int si = 1; si <= ctx.slavecount; si++) {
        ec_slavet* s = &ctx.slavelist[si];
        printf("Slave %d State=0x%02X AL=0x%04X %s  Obytes=%d Ibytes=%d\n",
               si, s->state, s->ALstatuscode,
               ec_ALstatuscode2string(s->ALstatuscode),
               s->Obytes, s->Ibytes);
    }
}

// SAFE-OPまで
static int wait_for_safeop()
{
    // 設定完了後、全体を SAFE-OP へ
    ctx.slavelist[0].state = EC_STATE_SAFE_OP;
    ecx_writestate(&ctx, 0);

    int st = ecx_statecheck(&ctx, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 3);
    ecx_readstate(&ctx);
    printf("All slaves requested SAFE-OP, statecheck=0x%02X (all=0x%02X)\n",
           st, ctx.slavelist[0].state);

    if ((ctx.slavelist[0].state & EC_STATE_SAFE_OP) == 0) {
        printf("Failed to reach SAFE-OP\n");
        dump_slave_states();
        return 0;
    }
    return 1;
}

// OPへ
static int request_op(int expectedWKC)
{
    // ★重要：SAFE-OP中にプロセスデータを回す（SMイベント/出力更新の“きっかけ”）
    for (int k = 0; k < 20; k++) {
        ecx_send_processdata(&ctx);
        ecx_receive_processdata(&ctx, EC_TIMEOUTRET);
        usleep(1000);
    }

    // 全スレーブへ OP 要求
    for (int i = 1; i <= ctx.slavecount; i++) {
        ctx.slavelist[i].state = EC_STATE_OPERATIONAL;
        ecx_writestate(&ctx, i);
    }

    int st = ecx_statecheck(&ctx, 0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE * 5);
    ecx_readstate(&ctx);
    printf("Requested OP, statecheck=0x%02X (all=0x%02X)\n",
           st, ctx.slavelist[0].state);

    if (ctx.slavelist[0].state != EC_STATE_OPERATIONAL) {
        printf("Failed to enter OP state\n");
        dump_slave_states();

        // 追加ヒント：WKCも出しておく
        ecx_send_processdata(&ctx);
        int wkc = ecx_receive_processdata(&ctx, EC_TIMEOUTRET);
        printf("WKC now=%d expected=%d\n", wkc, expectedWKC);

        return 0;
    }

    printf("Entered OP state\n");
    return 1;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: %s <ifname>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, on_sigint);

    const char* ifname = argv[1];
    printf("EtherCAT startup on interface: %s\n", ifname);

    if (!ecx_init(&ctx, (char*)ifname)) {
        printf("ecx_init failed (check ifname / permissions)\n");
        return 1;
    }

    int sc = ecx_config_init(&ctx);
    if (sc <= 0) {
        printf("No slaves found.\n");
        ecx_close(&ctx);
        return 1;
    }
    printf("%d slaves found and configured.\n", ctx.slavecount);

    // I/O map（DCは使わないので ecx_configdc は呼ばない）
    ecx_config_map_group(&ctx, IOmap, 0);

    while (ctx.ecaterror) {
        printf("%s\n", ecx_elist2string(&ctx));
    }

    ec_groupt* group = &ctx.grouplist[0];
    int expectedWKC = (group->outputsWKC * 2) + group->inputsWKC;
    printf("Calculated expected WKC: %d (outputsWKC=%d inputsWKC=%d)\n",
           expectedWKC, group->outputsWKC, group->inputsWKC);

    dump_slave_states();

    if (!wait_for_safeop()) {
        ecx_close(&ctx);
        return 1;
    }

    // PDOポインタ取得（SOEMは 1-indexed）
    if (ctx.slavecount < 1) {
        printf("Need at least 1 slave.\n");
        ecx_close(&ctx);
        return 1;
    }

    TxPDO_1i32* out = (TxPDO_1i32*)ctx.slavelist[1].outputs; // master->slave (SM2)
    RxPDO_1i32* in  = (RxPDO_1i32*)ctx.slavelist[1].inputs;  // slave->master (SM3)

    printf("Slave1 outputs ptr=%p inputs ptr=%p\n", (void*)out, (void*)in);
    printf("Slave1 Obytes=%d Ibytes=%d\n", ctx.slavelist[1].Obytes, ctx.slavelist[1].Ibytes);

    if (!out || !in) {
        printf("PDO pointers are null. Check slave PDO mapping.\n");
        dump_slave_states();
        ecx_close(&ctx);
        return 1;
    }
    if (ctx.slavelist[1].Obytes < (int)sizeof(TxPDO_1i32) ||
        ctx.slavelist[1].Ibytes < (int)sizeof(RxPDO_1i32)) {
        printf("PDO size mismatch: Obytes=%d need>=%zu, Ibytes=%d need>=%zu\n",
               ctx.slavelist[1].Obytes, sizeof(TxPDO_1i32),
               ctx.slavelist[1].Ibytes, sizeof(RxPDO_1i32));
        dump_slave_states();
        ecx_close(&ctx);
        return 1;
    }

    if (!request_op(expectedWKC)) {
        ecx_close(&ctx);
        return 1;
    }

    // 周期通信（DC/SYNC0無し＝単純な usleep で回す）
    int32_t cycle = 0;
    int miss = 0;

    while (g_run) {
        out->test_value_rx = cycle;

        ecx_send_processdata(&ctx);
        int wkc = ecx_receive_processdata(&ctx, EC_TIMEOUTRET);

        if (wkc != expectedWKC) {
            miss++;
            if (miss % 100 == 1) {
                printf("WKC mismatch: got=%d expected=%d (miss=%d)\n", wkc, expectedWKC, miss);
                dump_slave_states();
            }
        } else {
            miss = 0;
            int32_t r = in->test_value_tx;
            // 1000回に1回くらい表示（Serial系の重さで壊れないように）
            if ((cycle % 1000) == 0) {
                printf("[Master] cycle=%d, in(test_value_tx)=%d <-> [Slave] out(test_value_rx)=%d \n",
                       cycle, r, out->test_value_rx);
            }
        }

        cycle++;
        usleep(1000); // 1kHz 目安（適当に調整）
    }

    // 終了時：INITへ戻す（任意）
    ctx.slavelist[0].state = EC_STATE_INIT;
    ecx_writestate(&ctx, 0);
    ecx_close(&ctx);
    printf("Exit.\n");
    return 0;
}
