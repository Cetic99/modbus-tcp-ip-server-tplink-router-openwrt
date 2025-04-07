#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <modbus/modbus.h>

volatile bool running = true;

// Signal handler for SIGINT
void handle_sigint(int sig) {
    printf("\nCaught SIGINT (Ctrl+C). Cleaning up...\n");
    running = false;
}

void print_modbus_mapping(modbus_mapping_t *mb_mapping)
{
    printf("Modbus Mapping Details:\n");
    printf("  Coils:\n");
    printf("    Start Address: %d\n", mb_mapping->start_bits);
    printf("    Number: %d\n", mb_mapping->nb_bits);
    printf("  Input Bits:\n");
    printf("    Start Address: %d\n", mb_mapping->start_input_bits);
    printf("    Number: %d\n", mb_mapping->nb_input_bits);
    printf("  Input Registers:\n");
    printf("    Start Address: %d\n", mb_mapping->start_input_registers);
    printf("    Number: %d\n", mb_mapping->nb_input_registers);
    printf("  Holding Registers:\n");
    printf("    Start Address: %d\n", mb_mapping->start_registers);
    printf("    Number: %d\n", mb_mapping->nb_registers);
    fflush(stdout);
}

void print_status(modbus_mapping_t *mb_mapping)
{
    printf("Coils:      ");
    for (int i = 0; i < mb_mapping->nb_bits; i++) {
        printf("%d ", mb_mapping->tab_bits[i]);
    }
    printf("\n");
    
    printf("Input Bits: ");
    for (int i = 0; i < mb_mapping->nb_input_bits; i++) {
        printf("%d ", mb_mapping->tab_input_bits[i]);
    }
    printf("\n");

    printf("Holding Registers: ");
    for (int i = 0; i < mb_mapping->nb_registers; i++) {
        printf("%d ", mb_mapping->tab_registers[i]);
    }
    printf("\n");

    printf("Input Registers: ");
    for (int i = 0; i < mb_mapping->nb_input_registers; i++) {
        printf("%d ", mb_mapping->tab_input_registers[i]);
    }
    printf("\n");
}

// thread that print the status of the mapping every 100ms
void *print_status_thread(void *arg)
{
    modbus_mapping_t *mb_mapping = (modbus_mapping_t *)arg;
    while (running) {
        // clear the screen
        printf("\033[H\033[J");
        print_status(mb_mapping);
        fflush(stdout);
        usleep(100000); // Sleep for 100ms
    }
}

// thread that every half a second update value of the input register in ramp way
void *input_register_ramp_thread(void *arg)
{
    modbus_mapping_t *mb_mapping = (modbus_mapping_t *)arg;
    while (running) {
        for (int i = 0; i < mb_mapping->nb_input_registers; i++) {
            mb_mapping->tab_input_registers[i] += 1;
            if (mb_mapping->tab_input_registers[i] > 100) {
                mb_mapping->tab_input_registers[i] = 0;
            }
        }
        usleep(100000); // Sleep for 100ms
    }
}

// thread that shift data in the input bits
void *input_bits_shift_thread(void *arg)
{
    modbus_mapping_t *mb_mapping = (modbus_mapping_t *)arg;
    while (running) {
        int last = mb_mapping->tab_input_bits[mb_mapping->nb_input_bits - 1];
        for (int i = mb_mapping->nb_input_bits - 1; i > 0; i--) {
            mb_mapping->tab_input_bits[i] = mb_mapping->tab_input_bits[i - 1];
        }
        mb_mapping->tab_input_bits[0] = last; // Shift in a last value
        sleep(1); // Sleep for 1s
    }
}


int main() {
    pthread_t input_register_thread_id;
    pthread_t input_bits_thread_id;
    pthread_t print_status_thread_id;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    int server_socket;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

    signal(SIGINT, handle_sigint); // Register signal handler for SIGINT
    
    ctx = modbus_new_tcp("0.0.0.0", 502);
    if (ctx == NULL) {
        fprintf(stderr, "Unable to create Modbus TCP context\n");
        return -1;
    }

    mb_mapping = modbus_mapping_new(10, 10, 10, 10);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate modbus mapping\n");
        modbus_free(ctx);
        return -1;
    }
    // Print the initial mapping details
    print_modbus_mapping(mb_mapping);

    // Create a ramp thread to update input registers
    if (pthread_create(&input_register_thread_id, NULL, input_register_ramp_thread, mb_mapping) != 0) {
        fprintf(stderr, "Failed to create write thread\n");
        modbus_mapping_free(mb_mapping);
        modbus_free(ctx);
        return -1;
    }
    // Create a thread to shift input bits
    if (pthread_create(&input_bits_thread_id, NULL, input_bits_shift_thread, mb_mapping) != 0) {
        fprintf(stderr, "Failed to create input bits thread\n");
        modbus_mapping_free(mb_mapping);
        modbus_free(ctx);
        return -1;
    }
    // Create a thread to print the status
    if (pthread_create(&print_status_thread_id, NULL, print_status_thread, mb_mapping) != 0) {
        fprintf(stderr, "Failed to create print status thread\n");
        modbus_mapping_free(mb_mapping);
        modbus_free(ctx);
        return -1;
    }

    // Set initial values for the mapping
    // This is just an example, you can set your own values
    // Example: Set initial values
    for (int i = 0; i < 10; i++) {
        mb_mapping->tab_bits[i] = i % 2;            // Coil #i = ON for even, OFF for odd
        mb_mapping->tab_input_bits[i] = (i + 1) % 2; // Input Bit #i = ON for odd, OFF for even
        mb_mapping->tab_registers[i] = i * 10;       // Holding Register #i = i * 10
        mb_mapping->tab_input_registers[i] = 100 - i; // Input Register #i = 100 - i
    }

    // Print the initial status of the mapping
    print_status(mb_mapping);

    server_socket = modbus_tcp_listen(ctx, 1);
    modbus_tcp_accept(ctx, &server_socket);


    while (running) {
        int rc = modbus_receive(ctx, query);
        if (rc > 0)
        {
            modbus_reply(ctx, query, rc, mb_mapping);
        }
        else if (rc == -1)
        {
            // Connection lost, restart accept
            printf("Client disconnected. Waiting for new connection...\n");
            fflush(stdout);
            modbus_close(ctx);
            modbus_tcp_accept(ctx, &server_socket);  // Accept a new connection
        }
    }

    // Clean up
    pthread_join(input_register_thread_id, NULL); // Wait for the thread to finish
    pthread_join(input_bits_thread_id, NULL); // Wait for the thread to finish
    pthread_join(print_status_thread_id, NULL); // Wait for the thread to finish
    printf("Exiting...\n");
    fflush(stdout);

    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);
    return 0;
}