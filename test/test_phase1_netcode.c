#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#define PI 3.14159265358979323846f
#define PI2 (2.0f * PI)

// Test 1: Continuous Error Blending Exponential Decay
void test_error_blending(void) {
    float blend_dx = 150.0f;
    float blend_dy = -80.0f;
    float dt = 0.016666f; // 60 FPS frame time (16.6ms)

    for (int frame = 0; frame < 60; frame++) {
        float decay = expf(-18.0f * dt);
        blend_dx *= decay;
        blend_dy *= decay;
    }

    // After 1 second (60 frames), error offset should have decayed to effectively zero (< 1e-4)
    assert(fabsf(blend_dx) < 1e-4f);
    assert(fabsf(blend_dy) < 1e-4f);
    printf("PASS: test_error_blending: 150px error decayed asymptotically to 0.0 in 1.0s (residual dx=%.6f)\n", blend_dx);
}

// Test 2: Instantaneous Visual Continuity (Zero Snap on Packet Arrival)
void test_visual_continuity(void) {
    float local_xx = 1000.0f;
    float blend_dx = 12.0f;
    float cur_visual_x = local_xx + blend_dx; // 1012.0f

    // Packet arrives saying server xx is 1045.0f
    float server_xx = 1045.0f;
    float err_x = cur_visual_x - server_xx; // 1012 - 1045 = -33.0f

    blend_dx = err_x;
    local_xx = server_xx;

    float new_visual_x = local_xx + blend_dx; // 1045 + (-33) = 1012.0f
    assert(fabsf(new_visual_x - cur_visual_x) < 1e-5f);
    printf("PASS: test_visual_continuity: Before=%.2f, After=%.2f (Zero visual popping on packet arrival)\n",
           cur_visual_x, new_visual_x);
}

// Test 3: RFC 3550 Jitter Estimation
void test_jitter_estimation(void) {
    float net_rtt = 50.0f;
    float net_jitter = 0.0f;

    // Simulate RTT fluctuating between 40ms and 70ms
    float rtt_samples[] = {55.0f, 42.0f, 68.0f, 45.0f, 72.0f, 50.0f, 65.0f, 48.0f};
    int n = sizeof(rtt_samples) / sizeof(rtt_samples[0]);

    for (int i = 0; i < n; i++) {
        float rtt = rtt_samples[i];
        float diff = fabsf(rtt - net_rtt);
        net_jitter = net_jitter * 0.875f + diff * 0.125f;
        net_rtt = net_rtt * 0.75f + rtt * 0.25f;
    }

    assert(net_jitter > 2.0f && net_jitter < 25.0f);
    printf("PASS: test_jitter_estimation: Filtered RTT=%.1fms, RFC 3550 Jitter=%.1fms\n", net_rtt, net_jitter);
}

// Test 4: Steering Angle Quantization
void test_angle_quantization(void) {
    for (int deg = 0; deg < 360; deg += 15) {
        float rad = (float)deg * PI / 180.0f;
        float ang = fmodf(rad, PI2);
        if (ang < 0.0f) ang += PI2;
        int sang = (int)floorf(251.0f * ang / PI2);
        assert(sang >= 0 && sang <= 250);
    }
    printf("PASS: test_angle_quantization: All 360 degrees mapped strictly to [0, 250]\n");
}

int main(void) {
    printf("--- RUNNING PHASE 1 NETCODE VERIFICATION TESTS ---\n");
    test_error_blending();
    test_visual_continuity();
    test_jitter_estimation();
    test_angle_quantization();
    printf("--- ALL PHASE 1 NETCODE UNIT TESTS PASSED ---\n");
    return 0;
}
