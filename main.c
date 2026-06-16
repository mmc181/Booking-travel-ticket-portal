#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_NAME 50
#define MAX_SOURCE 50
#define MAX_DEST 50
#define MAX_TRANSPORTS 100
#define MAX_BOOKINGS 100
#define OUTPUT_BUFFER 8192
#define MAX_PATH_LEN 128

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
    char transportType[10];
    char seatNumber[5];
    char date[11];
} Booking;

enum {
    ID_VIEW_FLIGHTS = 101,
    ID_VIEW_TRAINS,
    ID_BOOK_FLIGHT,
    ID_BOOK_TRAIN,
    ID_CANCEL_TICKET,
    ID_VIEW_BOOKINGS,
    ID_EXIT,
    ID_POPUP_BOOK,
    ID_POPUP_CANCEL,
    ID_POPUP_CLOSE,
};

typedef struct {
    HWND hTransportList;
    HWND hNameEdit;
    HWND hSeatEdit;
    HWND hDateEdit;
    HWND hStatus;
    Transport transports[MAX_TRANSPORTS];
    int transportCount;
    char transportFile[MAX_PATH_LEN];
    char transportType[10];
    HWND hListBox;
    Booking bookings[MAX_BOOKINGS];
    int bookingCount;
} PopupContext;

static HINSTANCE g_hInst;
static HWND g_hOutput;
static HWND g_hMainWnd;
static char g_appDir[MAX_PATH];

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void GetDataFilePath(const char *filename, char *fullPath, int maxLen) {
    snprintf(fullPath, maxLen, "%s\\%s", g_appDir, filename);
}

bool LoadTransports(const char *filename, Transport transports[], int *count) {
    char fullPath[MAX_PATH];
    GetDataFilePath(filename, fullPath, sizeof(fullPath));
    FILE *file = fopen(fullPath, "r");
    if (!file) return false;

    *count = 0;
    while (*count < MAX_TRANSPORTS && fscanf(file, "%49[^,],%d,%49[^,],%49[^,],%d,%d\n",
                  transports[*count].name,
                  &transports[*count].number,
                  transports[*count].source,
                  transports[*count].destination,
                  &transports[*count].totalSeats,
                  &transports[*count].availableSeats) == 6) {
        (*count)++;
    }
    fclose(file);
    return true;
}

bool SaveTransports(const char *filename, Transport transports[], int count) {
    char fullPath[MAX_PATH];
    GetDataFilePath(filename, fullPath, sizeof(fullPath));
    FILE *file = fopen(fullPath, "w");
    if (!file) return false;

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s,%d,%s,%s,%d,%d\n",
                transports[i].name,
                transports[i].number,
                transports[i].source,
                transports[i].destination,
                transports[i].totalSeats,
                transports[i].availableSeats);
    }
    fclose(file);
    return true;
}

bool LoadBookings(Booking bookings[], int *count) {
    char fullPath[MAX_PATH];
    GetDataFilePath("bookings.dat", fullPath, sizeof(fullPath));
    FILE *file = fopen(fullPath, "r");
    if (!file) {
        *count = 0;
        return false;
    }

    *count = 0;
    char line[256];
    while (*count < MAX_BOOKINGS && fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%49[^,],%d,%9[^,],%4[^,],%10s\n",
                   bookings[*count].passengerName,
                   &bookings[*count].transportNumber,
                   bookings[*count].transportType,
                   bookings[*count].seatNumber,
                   bookings[*count].date) == 5) {
            (*count)++;
            continue;
        }
        if (sscanf(line, "%49[^,],%d,%4[^,],%10s\n",
                   bookings[*count].passengerName,
                   &bookings[*count].transportNumber,
                   bookings[*count].seatNumber,
                   bookings[*count].date) == 4) {
            strncpy(bookings[*count].transportType, "Unknown", sizeof(bookings[*count].transportType));
            bookings[*count].transportType[sizeof(bookings[*count].transportType) - 1] = '\0';
            (*count)++;
        }
    }
    fclose(file);
    return true;
}

bool SaveBookings(Booking bookings[], int count) {
    char fullPath[MAX_PATH];
    GetDataFilePath("bookings.dat", fullPath, sizeof(fullPath));
    FILE *file = fopen(fullPath, "w");
    if (!file) return false;

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s,%d,%s,%s,%s\n",
                bookings[i].passengerName,
                bookings[i].transportNumber,
                bookings[i].transportType,
                bookings[i].seatNumber,
                bookings[i].date);
    }
    fclose(file);
    return true;
}

bool AddBooking(const char *name, int transportNumber, const char *transportType, const char *seat, const char *date) {
    Booking bookings[MAX_BOOKINGS];
    int count = 0;
    LoadBookings(bookings, &count);
    if (count >= MAX_BOOKINGS) return false;

    Booking booking;
    strncpy(booking.passengerName, name, sizeof(booking.passengerName) - 1);
    booking.passengerName[sizeof(booking.passengerName) - 1] = '\0';
    booking.transportNumber = transportNumber;
    strncpy(booking.transportType, transportType, sizeof(booking.transportType) - 1);
    booking.transportType[sizeof(booking.transportType) - 1] = '\0';
    strncpy(booking.seatNumber, seat, sizeof(booking.seatNumber) - 1);
    booking.seatNumber[sizeof(booking.seatNumber) - 1] = '\0';
    strncpy(booking.date, date, sizeof(booking.date) - 1);
    booking.date[sizeof(booking.date) - 1] = '\0';

    bookings[count++] = booking;
    return SaveBookings(bookings, count);
}

bool AdjustTransportSeat(const char *filename, int transportNumber, int delta) {
    Transport transports[MAX_TRANSPORTS];
    int count = 0;
    if (!LoadTransports(filename, transports, &count)) return false;

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (transports[i].number == transportNumber) {
            transports[i].availableSeats += delta;
            if (transports[i].availableSeats < 0) transports[i].availableSeats = 0;
            if (transports[i].availableSeats > transports[i].totalSeats) transports[i].availableSeats = transports[i].totalSeats;
            found = true;
            break;
        }
    }

    if (!found) return false;
    return SaveTransports(filename, transports, count);
}

void FormatTransportText(const Transport transports[], int count, const char *type, char *buffer, int bufferSize) {
    snprintf(buffer, bufferSize,
             "%s list:\r\n%-4s %-15s %-5s %-12s %-12s %-7s %-7s\r\n",
             type,
             "No",
             "Name",
             "ID",
             "Source",
             "Destination",
             "Total",
             "Avail");
    strncat(buffer, "----------------------------------------------------------------\r\n", bufferSize - strlen(buffer) - 1);
    for (int i = 0; i < count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%-4d %-15s %-5d %-12s %-12s %-7d %-7d\r\n",
                 i + 1,
                 transports[i].name,
                 transports[i].number,
                 transports[i].source,
                 transports[i].destination,
                 transports[i].totalSeats,
                 transports[i].availableSeats);
        strncat(buffer, line, bufferSize - strlen(buffer) - 1);
    }
    if (count == 0) {
        strncat(buffer, "No entries available.\r\n", bufferSize - strlen(buffer) - 1);
    }
}

void FormatBookingText(const Booking bookings[], int count, char *buffer, int bufferSize) {
    snprintf(buffer, bufferSize,
             "Bookings:\r\n%-4s %-20s %-10s %-6s %-8s %-10s\r\n",
             "No",
             "Passenger",
             "Type",
             "ID",
             "Seat",
             "Date");
    strncat(buffer, "----------------------------------------------------------------\r\n", bufferSize - strlen(buffer) - 1);
    for (int i = 0; i < count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%-4d %-20s %-10s %-6d %-8s %-10s\r\n",
                 i + 1,
                 bookings[i].passengerName,
                 bookings[i].transportType,
                 bookings[i].transportNumber,
                 bookings[i].seatNumber,
                 bookings[i].date);
        strncat(buffer, line, bufferSize - strlen(buffer) - 1);
    }
    if (count == 0) {
        strncat(buffer, "No bookings found.\r\n", bufferSize - strlen(buffer) - 1);
    }
}

void SetOutputText(const char *text) {
    if (g_hOutput) {
        SetWindowTextA(g_hOutput, text);
    }
}

void ShowTransportData(const char *filename, const char *type) {
    Transport transports[MAX_TRANSPORTS];
    int count = 0;
    char buffer[OUTPUT_BUFFER] = {0};
    if (!LoadTransports(filename, transports, &count)) {
        snprintf(buffer, sizeof(buffer), "Unable to open %s data file.\r\n", type);
        SetOutputText(buffer);
        return;
    }
    FormatTransportText(transports, count, type, buffer, sizeof(buffer));
    SetOutputText(buffer);
}

void ShowBookingData(void) {
    Booking bookings[MAX_BOOKINGS];
    int count = 0;
    char buffer[OUTPUT_BUFFER] = {0};
    LoadBookings(bookings, &count);
    FormatBookingText(bookings, count, buffer, sizeof(buffer));
    SetOutputText(buffer);
}

HWND CreateBookingPopup(HWND parent, const char *transportFile, const char *transportType) {
    PopupContext *context = (PopupContext *)malloc(sizeof(PopupContext));
    if (!context) return NULL;
    memset(context, 0, sizeof(PopupContext));
    strncpy(context->transportFile, transportFile, sizeof(context->transportFile) - 1);
    strncpy(context->transportType, transportType, sizeof(context->transportType) - 1);

    HWND hwnd = CreateWindowExA(0,
                                "BookingPopup",
                                "Book Ticket",
                                WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                520,
                                380,
                                parent,
                                NULL,
                                g_hInst,
                                context);
    if (!hwnd) {
        free(context);
        return NULL;
    }
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}

HWND CreateCancelPopup(HWND parent) {
    PopupContext *context = (PopupContext *)malloc(sizeof(PopupContext));
    if (!context) return NULL;
    memset(context, 0, sizeof(PopupContext));

    HWND hwnd = CreateWindowExA(0,
                                "BookingPopup",
                                "Cancel Booking",
                                WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                560,
                                420,
                                parent,
                                NULL,
                                g_hInst,
                                context);
    if (!hwnd) {
        free(context);
        return NULL;
    }
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PopupContext *context = (PopupContext *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
            context = (PopupContext *)cs->lpCreateParams;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)context);
            if (!context) return -1;

            RECT rc;
            GetClientRect(hwnd, &rc);
            int left = 10;
            int top = 10;

            if (strlen(context->transportFile) > 0) {
                CreateWindowA("STATIC", "Select transport:", WS_CHILD | WS_VISIBLE,
                              left, top, 130, 20, hwnd, NULL, g_hInst, NULL);
                context->hTransportList = CreateWindowA("LISTBOX", NULL,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
                              left, top + 24, rc.right - 20, 140, hwnd, NULL, g_hInst, NULL);
                top += 170;
                CreateWindowA("STATIC", "Passenger name:", WS_CHILD | WS_VISIBLE,
                              left, top, 120, 20, hwnd, NULL, g_hInst, NULL);
                context->hNameEdit = CreateWindowA("EDIT", NULL,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                              left + 120, top, 220, 24, hwnd, NULL, g_hInst, NULL);
                top += 34;
                CreateWindowA("STATIC", "Seat number:", WS_CHILD | WS_VISIBLE,
                              left, top, 120, 20, hwnd, NULL, g_hInst, NULL);
                context->hSeatEdit = CreateWindowA("EDIT", NULL,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                              left + 120, top, 120, 24, hwnd, NULL, g_hInst, NULL);
                top += 34;
                CreateWindowA("STATIC", "Date (YYYY-MM-DD):", WS_CHILD | WS_VISIBLE,
                              left, top, 140, 20, hwnd, NULL, g_hInst, NULL);
                context->hDateEdit = CreateWindowA("EDIT", NULL,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                              left + 140, top, 140, 24, hwnd, NULL, g_hInst, NULL);
                top += 40;
                context->hStatus = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                              left, top, rc.right - 20, 20, hwnd, NULL, g_hInst, NULL);
                CreateWindowA("BUTTON", "Book", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              left, top + 30, 100, 30, hwnd, (HMENU)201, g_hInst, NULL);
                CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              left + 120, top + 30, 100, 30, hwnd, (HMENU)202, g_hInst, NULL);

                if (!LoadTransports(context->transportFile, context->transports, &context->transportCount)) {
                    SetWindowTextA(context->hStatus, "Unable to load transport data.");
                } else {
                    for (int i = 0; i < context->transportCount; i++) {
                        char label[128];
                        snprintf(label, sizeof(label), "%s #%d | %s -> %s | Seats %d/%d",
                                 context->transports[i].name,
                                 context->transports[i].number,
                                 context->transports[i].source,
                                 context->transports[i].destination,
                                 context->transports[i].availableSeats,
                                 context->transports[i].totalSeats);
                        SendMessageA(context->hTransportList, LB_ADDSTRING, 0, (LPARAM)label);
                        SendMessageA(context->hTransportList, LB_SETITEMDATA, i, i);
                    }
                }
            } else {
                CreateWindowA("STATIC", "Select a booking to cancel:", WS_CHILD | WS_VISIBLE,
                              left, top, 220, 20, hwnd, NULL, g_hInst, NULL);
                context->hListBox = CreateWindowA("LISTBOX", NULL,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
                              left, top + 24, rc.right - 20, 260, hwnd, NULL, g_hInst, NULL);
                top += 300;
                context->hStatus = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                              left, top, rc.right - 20, 20, hwnd, NULL, g_hInst, NULL);
                CreateWindowA("BUTTON", "Cancel Booking", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              left, top + 30, 140, 30, hwnd, (HMENU)211, g_hInst, NULL);
                CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              left + 160, top + 30, 100, 30, hwnd, (HMENU)212, g_hInst, NULL);

                LoadBookings(context->bookings, &context->bookingCount);
                for (int i = 0; i < context->bookingCount; i++) {
                    char label[128];
                    snprintf(label, sizeof(label), "%s | %s #%d | Seat %s | %s",
                             context->bookings[i].passengerName,
                             context->bookings[i].transportType,
                             context->bookings[i].transportNumber,
                             context->bookings[i].seatNumber,
                             context->bookings[i].date);
                    SendMessageA(context->hListBox, LB_ADDSTRING, 0, (LPARAM)label);
                    SendMessageA(context->hListBox, LB_SETITEMDATA, i, i);
                }
                if (context->bookingCount == 0) {
                    SetWindowTextA(context->hStatus, "No bookings available to cancel.");
                }
            }
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == 201 && context && strlen(context->transportFile) > 0) {
                int sel = (int)SendMessageA(context->hTransportList, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR) {
                    SetWindowTextA(context->hStatus, "Please select a transport before booking.");
                    break;
                }
                int index = (int)SendMessageA(context->hTransportList, LB_GETITEMDATA, sel, 0);
                if (index < 0 || index >= context->transportCount) {
                    SetWindowTextA(context->hStatus, "Invalid transport selection.");
                    break;
                }
                char passenger[MAX_NAME] = {0};
                char seat[5] = {0};
                char date[11] = {0};
                GetWindowTextA(context->hNameEdit, passenger, sizeof(passenger));
                GetWindowTextA(context->hSeatEdit, seat, sizeof(seat));
                GetWindowTextA(context->hDateEdit, date, sizeof(date));
                if (strlen(passenger) == 0 || strlen(seat) == 0 || strlen(date) == 0) {
                    SetWindowTextA(context->hStatus, "Please fill in all fields.");
                    break;
                }
                Transport *selected = &context->transports[index];
                if (selected->availableSeats <= 0) {
                    SetWindowTextA(context->hStatus, "No seats available for the selected transport.");
                    break;
                }
                selected->availableSeats -= 1;
                if (!SaveTransports(context->transportFile, context->transports, context->transportCount)) {
                    SetWindowTextA(context->hStatus, "Error saving transport data.");
                    break;
                }
                if (!AddBooking(passenger, selected->number, context->transportType, seat, date)) {
                    SetWindowTextA(context->hStatus, "Error saving booking.");
                    break;
                }
                char label[128];
                snprintf(label, sizeof(label), "%s #%d | %s -> %s | Seats %d/%d",
                         selected->name,
                         selected->number,
                         selected->source,
                         selected->destination,
                         selected->availableSeats,
                         selected->totalSeats);
                SendMessageA(context->hTransportList, LB_DELETESTRING, sel, 0);
                SendMessageA(context->hTransportList, LB_INSERTSTRING, sel, (LPARAM)label);
                SendMessageA(context->hTransportList, LB_SETITEMDATA, sel, index);
                SetWindowTextA(context->hStatus, "Ticket booked successfully.");
                SetWindowTextA(context->hNameEdit, "");
                SetWindowTextA(context->hSeatEdit, "");
                SetWindowTextA(context->hDateEdit, "");
            } else if (id == 202 || id == 212) {
                DestroyWindow(hwnd);
            } else if (id == 211 && context && strlen(context->transportFile) == 0) {
                int sel = (int)SendMessageA(context->hListBox, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR) {
                    SetWindowTextA(context->hStatus, "Select a booking to cancel.");
                    break;
                }
                int index = (int)SendMessageA(context->hListBox, LB_GETITEMDATA, sel, 0);
                if (index < 0 || index >= context->bookingCount) {
                    SetWindowTextA(context->hStatus, "Invalid booking selection.");
                    break;
                }
                Booking removed = context->bookings[index];
                for (int i = index; i < context->bookingCount - 1; i++) {
                    context->bookings[i] = context->bookings[i + 1];
                }
                context->bookingCount -= 1;
                if (!SaveBookings(context->bookings, context->bookingCount)) {
                    SetWindowTextA(context->hStatus, "Error saving booking data.");
                    break;
                }
                SendMessageA(context->hListBox, LB_DELETESTRING, sel, 0);
                SetWindowTextA(context->hStatus, "Booking canceled successfully.");
                if (strcmp(removed.transportType, "Flight") == 0) {
                    AdjustTransportSeat("flights.dat", removed.transportNumber, 1);
                } else if (strcmp(removed.transportType, "Train") == 0) {
                    AdjustTransportSeat("trains.dat", removed.transportNumber, 1);
                }
            }
            break;
        }

        case WM_DESTROY:
            if (context) free(context);
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hMainWnd = hwnd;
            g_hOutput = CreateWindowExA(WS_EX_CLIENTEDGE,
                                       "EDIT",
                                       "",
                                       WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_HSCROLL,
                                       10, 10, 560, 280,
                                       hwnd,
                                       NULL,
                                       g_hInst,
                                       NULL);
            CreateWindowA("BUTTON", "View Flights", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         10, 305, 130, 30, hwnd, (HMENU)ID_VIEW_FLIGHTS, g_hInst, NULL);
            CreateWindowA("BUTTON", "View Trains", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         150, 305, 130, 30, hwnd, (HMENU)ID_VIEW_TRAINS, g_hInst, NULL);
            CreateWindowA("BUTTON", "Book Flight", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         290, 305, 130, 30, hwnd, (HMENU)ID_BOOK_FLIGHT, g_hInst, NULL);
            CreateWindowA("BUTTON", "Book Train", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         430, 305, 130, 30, hwnd, (HMENU)ID_BOOK_TRAIN, g_hInst, NULL);
            CreateWindowA("BUTTON", "Cancel Ticket", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         10, 345, 130, 30, hwnd, (HMENU)ID_CANCEL_TICKET, g_hInst, NULL);
            CreateWindowA("BUTTON", "View Bookings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         150, 345, 130, 30, hwnd, (HMENU)ID_VIEW_BOOKINGS, g_hInst, NULL);
            CreateWindowA("BUTTON", "Exit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         430, 345, 130, 30, hwnd, (HMENU)ID_EXIT, g_hInst, NULL);
            SetOutputText("Welcome to the Booking Portal. Use the buttons below to view and manage bookings.\r\n");
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_VIEW_FLIGHTS:
                    ShowTransportData("flights.dat", "Flights");
                    break;
                case ID_VIEW_TRAINS:
                    ShowTransportData("trains.dat", "Trains");
                    break;
                case ID_BOOK_FLIGHT:
                    CreateBookingPopup(hwnd, "flights.dat", "Flight");
                    break;
                case ID_BOOK_TRAIN:
                    CreateBookingPopup(hwnd, "trains.dat", "Train");
                    break;
                case ID_CANCEL_TICKET:
                    CreateCancelPopup(hwnd);
                    break;
                case ID_VIEW_BOOKINGS:
                    ShowBookingData();
                    break;
                case ID_EXIT:
                    PostMessageA(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    g_hInst = hInstance;

    // Initialize application directory path
    char exePath[MAX_PATH];
    char *lastSlash;
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    lastSlash = strrchr(exePath, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
        strncpy(g_appDir, exePath, sizeof(g_appDir) - 1);
        g_appDir[sizeof(g_appDir) - 1] = '\0';
    } else {
        strcpy(g_appDir, ".");
    }

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "BookingMainWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);

    WNDCLASSA popupClass = {0};
    popupClass.lpfnWndProc = PopupWndProc;
    popupClass.hInstance = hInstance;
    popupClass.lpszClassName = "BookingPopup";
    popupClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    popupClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&popupClass);

    HWND hwnd = CreateWindowExA(0,
                                "BookingMainWindow",
                                "Booking Portal",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                600,
                                420,
                                NULL,
                                NULL,
                                hInstance,
                                NULL);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
