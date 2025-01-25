#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_PRODUCTS 20
#define MAX_DESC_LEN 50
#define MAX_CLIENTS 5

// Struct to represent a product
struct product {
    char description[MAX_DESC_LEN];
    float price;
    int item_count;
    int total_orders;
    int successful_orders;
    int failed_orders;
};

struct product catalog[NUM_PRODUCTS];

// Function prototypes
void init_products();
void process_order(int client_socket);
void generate_report();

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Initialize the product catalog
    init_products();

    // Create the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        // Accept a new client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Client accept failed");
            continue;
        }

        // Fork a process to handle the client
        if (fork() == 0) {
            close(server_socket); // Child process closes server socket
            process_order(client_socket);
            close(client_socket); // Close client socket after processing
            exit(0);
        }
        close(client_socket); // Parent process closes client socket
    }

    // Wait for all child processes to complete
    for (int i = 0; i < MAX_CLIENTS; i++) {
        wait(NULL);
    }

    // Generate the final report
    generate_report();

    // Close the server socket
    close(server_socket);
    return 0;
}

// Initialize the product catalog
void init_products() {
    srand(time(NULL));
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        snprintf(catalog[i].description, MAX_DESC_LEN, "Product %d", i + 1);
        catalog[i].price = 10.0 + (rand() % 91); // Random price between 10.0 and 100.0
        catalog[i].item_count = 1 + (rand() % 5); // Random stock between 1 and 5
        catalog[i].total_orders = 0;
        catalog[i].successful_orders = 0;
        catalog[i].failed_orders = 0;
    }
}

// Process an order from a client
void process_order(int client_socket) {
    char buffer[256];
    for (int i = 0; i < 10; i++) { // Simulate 10 orders per client
        int product_index = rand() % NUM_PRODUCTS;
        int quantity_ordered = 1 + (rand() % 5);

        snprintf(buffer, sizeof(buffer), "Customer requested Product %d, Quantity: %d", product_index + 1, quantity_ordered);
        send(client_socket, buffer, strlen(buffer) + 1, 0);

        // Process the order
        catalog[product_index].total_orders++;
        if (catalog[product_index].item_count >= quantity_ordered) {
            catalog[product_index].item_count -= quantity_ordered;
            catalog[product_index].successful_orders++;
            snprintf(buffer, sizeof(buffer), "Order Success: Product %d, Quantity: %d", product_index + 1, quantity_ordered);
        } else {
            catalog[product_index].failed_orders++;
            snprintf(buffer, sizeof(buffer), "Order Failed: Product %d, Out of stock", product_index + 1);
        }
        send(client_socket, buffer, strlen(buffer) + 1, 0);
        sleep(1); // Simulate processing delay
    }
}

// Generate a sales report
void generate_report() {
    printf("\n--- Sales Report ---\n");
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        printf("\nProduct: %s\n", catalog[i].description);
        printf("Price: %.2f\n", catalog[i].price);
        printf("Stock Remaining: %d\n", catalog[i].item_count);
        printf("Total Orders: %d\n", catalog[i].total_orders);
        printf("Successful Orders: %d\n", catalog[i].successful_orders);
        printf("Failed Orders: %d\n", catalog[i].failed_orders);
    }
}
