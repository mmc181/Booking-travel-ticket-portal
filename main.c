#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_NAME 50
#define MAX_SOURCE 50
#define MAX_DEST 50
#define MAX_BOOKINGS 100

typedef struct {
    char name[MAX_NAME];
    int number;
    char source[MAX_SOURCE];
    char destination[MAX_DEST];
    int totalSeats;
    int availableSeats;
} Transport;

typedef struct {
    char passengerName[MAX_NAME];
    int transportNumber;
    char seatNumber[5];
    char date[11];
} Booking;

// Function prototypes
void viewTransports(const char *filename, const char *type);
void bookTicket(const char *transportFile, const char *bookingFile, const char *type);
void cancelTicket(const char *bookingFile);
void viewBookings(const char *bookingFile);
void adminPanel(const char *filename, const char *type);
void displaySeatMap(int totalSeats, int availableSeats);

int main() {
    int choice;
    do {
        printf("\n=== SEATING ARRANGEMENT SYSTEM ===\n");
        printf("1. View Available Flights\n");
        printf("2. View Available Trains\n");
        printf("3. Book a Ticket\n");
        printf("4. Cancel a Ticket\n");
        printf("5. View Booking History\n");
        printf("6. Admin Panel\n");
        printf("7. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Consume newline

        switch (choice) {
            case 1: viewTransports("flights.dat", "Flight"); break;
            case 2: viewTransports("trains.dat", "Train"); break;
            case 3:
                printf("1. Book Flight\n2. Book Train\n");
                int bookChoice; scanf("%d", &bookChoice);
                if (bookChoice == 1) bookTicket("flights.dat", "bookings.dat", "Flight");
                else if (bookChoice == 2) bookTicket("trains.dat", "bookings.dat", "Train");
                break;
            case 4: cancelTicket("bookings.dat"); break;
            case 5: viewBookings("bookings.dat"); break;
            case 6:
                printf("1. Admin: Flights\n2. Admin: Trains\n");
                int adminChoice; scanf("%d", &adminChoice);
                if (adminChoice == 1) adminPanel("flights.dat", "Flight");
                else if (adminChoice == 2) adminPanel("trains.dat", "Train");
                break;
            case 7: exit(0);
            default: printf("Invalid choice!\n");
        }
    } while (1);
    return 0;
}

void viewTransports(const char *filename, const char *type) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening %s database!\n", type);
        return;
    }
    Transport t;
    printf("\nAvailable %ss:\n", type);
    printf("--------------------------------------------------\n");
    while (fscanf(file, "%49[^,],%d,%49[^,],%49[^,],%d,%d\n",
                  t.name, &t.number, t.source, t.destination, &t.totalSeats, &t.availableSeats) != EOF) {
        printf("%s (%d): %s to %s | Seats: %d/%d\n",
               t.name, t.number, t.source, t.destination, t.availableSeats, t.totalSeats);
    }
    fclose(file);
}

void bookTicket(const char *transportFile, const char *bookingFile, const char *type) {
    FILE *tFile = fopen(transportFile, "r+");
    if (!tFile) {
        printf("Error opening %s database!\n", type);
        return;
    }
    Transport transports[100];
    int count = 0;
    while (fscanf(tFile, "%49[^,],%d,%49[^,],%49[^,],%d,%d\n",
                  transports[count].name, &transports[count].number,
                  transports[count].source, transports[count].destination,
                  &transports[count].totalSeats, &transports[count].availableSeats) != EOF) {
        count++;
    }
    fclose(tFile);

    printf("\nSelect a %s:\n", type);
    for (int i = 0; i < count; i++) {
        printf("%d. %s (%d): %s to %s | Seats: %d/%d\n",
               i+1, transports[i].name, transports[i].number,
               transports[i].source, transports[i].destination,
               transports[i].availableSeats, transports[i].totalSeats);
    }
    int selection;
    printf("Enter your choice: ");
    scanf("%d", &selection);
    getchar();
    if (selection < 1 || selection > count) {
        printf("Invalid selection!\n");
        return;
    }
    Transport selected = transports[selection-1];
    if (selected.availableSeats <= 0) {
        printf("No seats available!\n");
        return;
    }

    displaySeatMap(selected.totalSeats, selected.availableSeats);
    printf("Enter seat number (e.g., A1): ");
    char seatNumber[5];
    scanf("%4s", seatNumber);
    getchar();

    printf("Enter passenger name: ");
    char passengerName[MAX_NAME];
    fgets(passengerName, MAX_NAME, stdin);
    passengerName[strcspn(passengerName, "\n")] = '\0';

    printf("Enter date (YYYY-MM-DD): ");
    char date[11];
    scanf("%10s", date);
    getchar();

    // Update transport file
    tFile = fopen(transportFile, "r+");
    FILE *tempFile = fopen("temp.dat", "w");
    Transport t;
    while (fscanf(tFile, "%49[^,],%d,%49[^,],%49[^,],%d,%d\n",
                  t.name, &t.number, t.source, t.destination, &t.totalSeats, &t.availableSeats) != EOF) {
        if (t.number == selected.number) {
            t.availableSeats--;
            fprintf(tempFile, "%s,%d,%s,%s,%d,%d\n",
                    t.name, t.number, t.source, t.destination, t.totalSeats, t.availableSeats);
        } else {
            fprintf(tempFile, "%s,%d,%s,%s,%d,%d\n",
                    t.name, t.number, t.source, t.destination, t.totalSeats, t.availableSeats);
        }
    }
    fclose(tFile);
    fclose(tempFile);
    remove(transportFile);
    rename("temp.dat", transportFile);

    // Add booking
    FILE *bFile = fopen(bookingFile, "a");
    fprintf(bFile, "%s,%d,%s,%s\n", passengerName, selected.number, seatNumber, date);
    fclose(bFile);

    printf("Ticket booked successfully!\n");
}

void cancelTicket(const char *bookingFile) {
    FILE *bFile = fopen(bookingFile, "r");
    if (!bFile) {
        printf("Error opening bookings database!\n");
        return;
    }
    Booking bookings[MAX_BOOKINGS];
    int count = 0;
    while (fscanf(bFile, "%49[^,],%d,%4[^,],%10s\n",
                  bookings[count].passengerName, &bookings[count].transportNumber,
                  bookings[count].seatNumber, bookings[count].date) != EOF) {
        count++;
    }
    fclose(bFile);

    printf("\nYour Bookings:\n");
    for (int i = 0; i < count; i++) {
        printf("%d. %s: %s (Seat: %s, Date: %s)\n",
               i+1, bookings[i].passengerName, bookings[i].transportNumber == 1 ? "Flight" : "Train",
               bookings[i].seatNumber, bookings[i].date);
    }
    int selection;
    printf("Enter booking number to cancel: ");
    scanf("%d", &selection);
    getchar();
    if (selection < 1 || selection > count) {
        printf("Invalid selection!\n");
        return;
    }
    Booking toCancel = bookings[selection-1];

    // Remove booking
    bFile = fopen(bookingFile, "r");
    FILE *tempFile = fopen("temp.dat", "w");
    Booking b;
    while (fscanf(bFile, "%49[^,],%d,%4[^,],%10s\n",
                  b.passengerName, &b.transportNumber, b.seatNumber, b.date) != EOF) {
        if (strcmp(b.passengerName, toCancel.passengerName) != 0 ||
            b.transportNumber != toCancel.transportNumber ||
            strcmp(b.seatNumber, toCancel.seatNumber) != 0 ||
            strcmp(b.date, toCancel.date) != 0) {
            fprintf(tempFile, "%s,%d,%s,%s\n", b.passengerName, b.transportNumber, b.seatNumber, b.date);
        }
    }
    fclose(bFile);
    fclose(tempFile);
    remove(bookingFile);
    rename("temp.dat", bookingFile);

    // Update transport file (increase available seats)
    char transportFile[20];
    sprintf(transportFile, "%s.dat", toCancel.transportNumber == 1 ? "flights" : "trains");
    FILE *tFile = fopen(transportFile, "r");
    FILE *tTempFile = fopen("temp.dat", "w");
    Transport t;
    while (fscanf(tFile, "%49[^,],%d,%49[^,],%49[^,],%d,%d\n",
                  t.name, &t.number, t.source, t.destination, &t.totalSeats, &t.availableSeats) != EOF) {
        if (t.number == toCancel.transportNumber) {
            t.availableSeats++;
        }
        fprintf(tTempFile, "%s,%d,%s,%s,%d,%d\n",
                t.name, t.number, t.source, t.destination, t.totalSeats, t.availableSeats);
    }
    fclose(tFile);
    fclose(tTempFile);
    remove(transportFile);
    rename("temp.dat", transportFile);

    printf("Ticket canceled successfully!\n");
}

void viewBookings(const char *bookingFile) {
    FILE *file = fopen(bookingFile, "r");
    if (!file) {
        printf("Error opening bookings database!\n");
        return;
    }
    Booking b;
    printf("\nYour Bookings:\n");
    printf("--------------------------------------------------\n");
    while (fscanf(file, "%49[^,],%d,%4[^,],%10s\n",
                  b.passengerName, &b.transportNumber, b.seatNumber, b.date) != EOF) {
        printf("%s: %s (Seat: %s, Date: %s)\n",
               b.passengerName, b.transportNumber == 1 ? "Flight" : "Train", b.seatNumber, b.date);
    }
    fclose(file);
}

void adminPanel(const char *filename, const char *type) {
    printf("\nAdmin Panel: %s\n", type);
    printf("1. Add %s\n", type);
    printf("2. Remove %s\n", type);
    printf("3. Back\n");
    int choice;
    scanf("%d", &choice);
    getchar();

    if (choice == 1) {
        Transport t;
        printf("Enter %s name: ", type);
        fgets(t.name, MAX_NAME, stdin);
        t.name[strcspn(t.name, "\n")] = '\0';
        printf("Enter %s number: ", type);
        scanf("%d", &t.number);
        getchar();
        printf("Enter source: ");
        fgets(t.source, MAX_SOURCE, stdin);
        t.source[strcspn(t.source, "\n")] = '\0';
        printf("Enter destination: ");
        fgets(t.destination, MAX_DEST, stdin);
        t.destination[strcspn(t.destination, "\n")] = '\0';
        printf("Enter total seats: ");
        scanf("%d", &t.totalSeats);
        t.availableSeats = t.totalSeats;
        getchar();

        FILE *file = fopen(filename, "a");
        fprintf(file, "%s,%d,%s,%s,%d,%d\n",
                t.name, t.number, t.source, t.destination, t.totalSeats, t.availableSeats);
        fclose(file);
        printf("%s added successfully!\n", type);
    } else if (choice == 2) {
        viewTransports(filename, type);
        int number;
        printf("Enter %s number to remove: ", type);
        scanf("%d", &number);
        getchar();

        FILE *file = fopen(filename, "r");
        FILE *tempFile = fopen("temp.dat", "w");
        Transport t;
        while (fscanf(file, "%49[^,],%d,%49[^,],%49[^,],%d,%d\n",
                      t.name, &t.number, t.source, t.destination, &t.totalSeats, &t.availableSeats) != EOF) {
            if (t.number != number) {
                fprintf(tempFile, "%s,%d,%s,%s,%d,%d\n",
                        t.name, t.number, t.source, t.destination, t.totalSeats, t.availableSeats);
            }
        }
        fclose(file);
        fclose(tempFile);
        remove(filename);
        rename("temp.dat", filename);
        printf("%s removed successfully!\n", type);
    }
}

void displaySeatMap(int totalSeats, int availableSeats) {
    int rows = totalSeats / 4;
    int taken = totalSeats - availableSeats;
    printf("\nSeat Map (A=Available, X=Taken):\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 4; j++) {
            int seatNum = i * 4 + j + 1;
            if (seatNum <= taken) printf("X ");
            else printf("A ");
        }
        printf("\n");
    }
}
