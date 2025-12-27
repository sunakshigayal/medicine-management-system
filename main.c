/*
 medicine_tracker.c
 Single-file Medicine Tracker
 - Register (default STAFF)
 - Login: user chooses Staff or Admin after credential check; Admin choice can promote user with secret
 - Pipe-separated medicines file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ADMIN_SECRET "admin123"   /* secret to promote to admin */

typedef struct User {
    char username[32];
    char password[32];
    int role; /* 0 = STAFF, 1 = ADMIN */
    struct User *next;
} User;

typedef struct Medicine {
    char batchNumber[32];
    char brandName[64];
    char genericName[64];
    char manufacturer[64];
    char manufacturedDate[16]; /* YYYY-MM-DD */
    char expiryDate[16];       /* YYYY-MM-DD */
    int quantity;
    char status[24];
    struct Medicine *next;
} Medicine;

/* Globals */
User *userHead = NULL;
Medicine *medHead = NULL;

const char *USER_FILE = "users.txt";
const char *MED_FILE  = "medicines.txt";

/* Prototypes */
/* Users */
void loadUsers(void);
void appendUserToFile(User *u);
void rewriteUsersFile(void);
int isDuplicateUser(const char *uname);
void registerUser(void);
User* findUser(const char *uname);
User* login_flow(void); /* combined login flow described */
void ensure_default_admin(void);

/* Medicines */
void loadMedicines(void);
void saveMedicines(void);
Medicine* searchMedicine(const char *batch);
void addMedicine(void);
void displayMedicines(void);
void updateMedicine(void);
void deleteMedicine(void);
int compareDate(const char *d1, const char *d2); /* YYYY-MM-DD compare */
Medicine* sortedInsertByExpiry(Medicine* head, Medicine* newNode);
void sortMedicinesByExpiry(void);

/* Status & expiry */
void updateExpiryAndStockStatus(Medicine *m);
void updateAllMedicineStatuses(void);
void checkExpiryStatus(void);

/* Stats */
void medicineStats(void);

/* Menus */
void adminMenu(User *u);
void staffMenu(User *u);

/* Utils */
void strip_newline(char *s);

/* ---------------- Implementation ---------------- */

void strip_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    if (n && s[n-1] == '\n') s[n-1] = '\0';
}

/* ---------- Users ---------- */

void loadUsers(void) {
    FILE *fp = fopen(USER_FILE, "r");
    if (!fp) return;
    while (1) {
        char uname[32], pass[32], roleStr[16];
        if (fscanf(fp, "%31s %31s %15s\n", uname, pass, roleStr) != 3) break;
        User *u = malloc(sizeof(User));
        if (!u) break;
        strcpy(u->username, uname);
        strcpy(u->password, pass);
        u->role = (strcmp(roleStr, "ADMIN") == 0) ? 1 : 0;
        u->next = userHead;
        userHead = u;
    }
    fclose(fp);
}

/* Append single user line to file (used on registration) */
void appendUserToFile(User *u) {
    if (!u) return;
    FILE *fp = fopen(USER_FILE, "a");
    if (!fp) return;
    fprintf(fp, "%s %s %s\n", u->username, u->password, (u->role==1) ? "ADMIN" : "STAFF");
    fclose(fp);
}

/* Rewrite entire users file from linked list (used after promotion) */
void rewriteUsersFile(void) {
    FILE *fp = fopen(USER_FILE, "w");
    if (!fp) { perror("Unable to write users file"); return; }
    User *t = userHead;
    while (t) {
        fprintf(fp, "%s %s %s\n", t->username, t->password, (t->role==1) ? "ADMIN" : "STAFF");
        t = t->next;
    }
    fclose(fp);
}

int isDuplicateUser(const char *uname) {
    User *t = userHead;
    while (t) {
        if (strcmp(t->username, uname) == 0) return 1;
        t = t->next;
    }
    return 0;
}

void registerUser(void) {
    char uname[32], pass[32];
    printf("\n=== Register (new user becomes STAFF) ===\n");
    printf("Username: ");
    scanf("%31s", uname);
    if (strlen(uname) == 0) { printf("Invalid username.\n"); return; }
    if (isDuplicateUser(uname)) { printf("Username already exists.\n"); return; }
    printf("Password: ");
    scanf("%31s", pass);

    User *u = malloc(sizeof(User));
    if (!u) { printf("Memory error\n"); return; }
    strcpy(u->username, uname);
    strcpy(u->password, pass);
    u->role = 0; /* staff */
    u->next = userHead;
    userHead = u;
    appendUserToFile(u);
    printf("Registered successfully as STAFF. You can now login.\n");
}

User* findUser(const char *uname) {
    User *t = userHead;
    while (t) {
        if (strcmp(t->username, uname) == 0) return t;
        t = t->next;
    }
    return NULL;
}

/* Combined login flow:
   - ask username & password; verify credentials
   - ask user which role to login as (staff/admin)
   - if admin chosen and user is not admin, ask admin secret; if correct -> promote (persist), else deny admin
   - returns pointer to User if login success (role will reflect chosen/promoted role)
*/
User* login_flow(void) {
    char uname[32], pass[32];
    printf("\n=== Login ===\nUsername: ");
    scanf("%31s", uname);
    printf("Password: ");
    scanf("%31s", pass);

    User *u = findUser(uname);
    if (!u || strcmp(u->password, pass) != 0) {
        printf("Invalid username or password!\n");
        return NULL;
    }

    /* ask user which role they want to login as */
    int choice;
    printf("Login as: 1. Staff   2. Admin\nChoice: ");
    if (scanf("%d", &choice) != 1) { while (getchar()!='\n'); return NULL; }

    if (choice == 1) {
        /* If user was admin, allow to login as staff as well */
        printf("Logged in as STAFF.\n");
        u->role = u->role; /* keep role as-is; treat session as staff */
        return u;
    } else if (choice == 2) {
        if (u->role == 1) {
            printf("Logged in as ADMIN.\n");
            return u;
        } else {
            /* prompt secret to promote */
            char secret[64];
            printf("Enter Admin Secret to elevate your account: ");
            scanf("%63s", secret);
            if (strcmp(secret, ADMIN_SECRET) == 0) {
                /* promote in-memory and persist */
                u->role = 1;
                rewriteUsersFile();
                printf("Admin secret accepted. Account promoted to ADMIN. Logged in as ADMIN.\n");
                return u;
            } else {
                printf("Wrong admin secret. Logging in as STAFF instead.\n");
                return u; /* return as staff session */
            }
        }
    } else {
        printf("Invalid choice.\n");
        return NULL;
    }
}

void ensure_default_admin(void) {
    if (userHead != NULL) return;
    User *a = malloc(sizeof(User));
    if (!a) return;
    strcpy(a->username, "admin");
    strcpy(a->password, "admin123");
    a->role = 1;
    a->next = userHead;
    userHead = a;
    appendUserToFile(a);
    printf("Default admin created: username 'admin' password 'admin123'\n");
}

/* ---------- Medicines (pipe-separated) ---------- */

void loadMedicines(void) {
    FILE *fp = fopen(MED_FILE, "r");
    if (!fp) return;
    while (1) {
        Medicine *m = malloc(sizeof(Medicine));
        if (!m) break;
        /* parse:
           batchNumber|brandName|genericName|manufacturer|manufacturedDate|expiryDate|quantity|status
        */
        int scanned = fscanf(fp, "%31[^|]|%63[^|]|%63[^|]|%63[^|]|%15[^|]|%15[^|]|%d|%23[^\n]\n",
                             m->batchNumber, m->brandName, m->genericName, m->manufacturer,
                             m->manufacturedDate, m->expiryDate, &m->quantity, m->status);
        if (scanned != 8) { free(m); break; }
        m->next = medHead;
        medHead = m;
    }
    fclose(fp);
    updateAllMedicineStatuses();
}

void saveMedicines(void) {
    FILE *fp = fopen(MED_FILE, "w");
    if (!fp) { perror("Unable to save medicines"); return; }
    Medicine *t = medHead;
    while (t) {
        fprintf(fp, "%s|%s|%s|%s|%s|%s|%d|%s\n",
                t->batchNumber, t->brandName, t->genericName, t->manufacturer,
                t->manufacturedDate, t->expiryDate, t->quantity, t->status);
        t = t->next;
    }
    fclose(fp);
}

/* date compare for YYYY-MM-DD */
int compareDate(const char *d1, const char *d2) {
    int y1=0,m1=0,day1=0,y2=0,m2=0,day2=0;
    if (sscanf(d1, "%d-%d-%d", &y1,&m1,&day1) != 3) return strcmp(d1,d2);
    if (sscanf(d2, "%d-%d-%d", &y2,&m2,&day2) != 3) return strcmp(d1,d2);
    if (y1!=y2) return y1 - y2;
    if (m1!=m2) return m1 - m2;
    return day1 - day2;
}

Medicine* sortedInsertByExpiry(Medicine* head, Medicine* newNode) {
    if (!head || compareDate(newNode->expiryDate, head->expiryDate) < 0) {
        newNode->next = head;
        return newNode;
    }
    Medicine *cur = head;
    while (cur->next && compareDate(cur->next->expiryDate, newNode->expiryDate) <= 0)
        cur = cur->next;
    newNode->next = cur->next;
    cur->next = newNode;
    return head;
}

void sortMedicinesByExpiry(void) {
    Medicine *sorted = NULL;
    Medicine *cur = medHead;
    while (cur) {
        Medicine *next = cur->next;
        cur->next = NULL;
        sorted = sortedInsertByExpiry(sorted, cur);
        cur = next;
    }
    medHead = sorted;
}

/* Status precedence */
void updateExpiryAndStockStatus(Medicine *m) {
    if (!m) return;
    int y=0,mon=0,day=0;
    if (sscanf(m->expiryDate, "%d-%d-%d", &y,&mon,&day) == 3) {
        struct tm exp = {0};
        exp.tm_year = y - 1900;
        exp.tm_mon  = mon - 1;
        exp.tm_mday = day;
        time_t exp_time = mktime(&exp);
        time_t now = time(NULL);
        double days = difftime(exp_time, now) / (60.0*60.0*24.0);
        if (days < 0.0) { strcpy(m->status, "EXPIRED"); return; }
        if (days <= 30.0) { strcpy(m->status, "NEAR EXPIRY"); return; }
    }
    if (m->quantity <= 0) strcpy(m->status, "OUT OF STOCK");
    else if (m->quantity <= 10) strcpy(m->status, "LOW STOCK");
    else strcpy(m->status, "IN STOCK");
}

void updateAllMedicineStatuses(void) {
    Medicine *t = medHead;
    while (t) {
        updateExpiryAndStockStatus(t);
        t = t->next;
    }
    saveMedicines();
}

void checkExpiryStatus(void) {
    updateAllMedicineStatuses();
    Medicine *t = medHead;
    printf("\n--- Expiry Status Report ---\n");
    while (t) {
        printf("Batch: %s | Brand: %s | Expiry: %s | Qty: %d | Status: %s\n",
               t->batchNumber, t->brandName, t->expiryDate, t->quantity, t->status);
        t = t->next;
    }
    printf("-------------------------------\n");
}

/* ---------- Medicine CRUD ---------- */

Medicine* searchMedicine(const char *batch) {
    Medicine *t = medHead;
    while (t) {
        if (strcmp(t->batchNumber, batch) == 0) return t;
        t = t->next;
    }
    return NULL;
}

void addMedicine(void) {
    Medicine *m = malloc(sizeof(Medicine));
    if (!m) { printf("Memory error\n"); return; }
    printf("\nEnter batch number (no spaces): "); scanf("%31s", m->batchNumber);
    if (searchMedicine(m->batchNumber)) { printf("Batch exists\n"); free(m); return; }
    printf("Brand name (no spaces): "); scanf("%63s", m->brandName);
    printf("Generic name (no spaces): "); scanf("%63s", m->genericName);
    printf("Manufacturer (no spaces): "); scanf("%63s", m->manufacturer);
    printf("Manufactured date (YYYY-MM-DD): "); scanf("%15s", m->manufacturedDate);
    printf("Expiry date (YYYY-MM-DD): "); scanf("%15s", m->expiryDate);
    printf("Quantity: "); scanf("%d", &m->quantity);
    updateExpiryAndStockStatus(m);
    m->next = medHead;
    medHead = m;
    sortMedicinesByExpiry();
    saveMedicines();
    printf("Added and sorted.\n");
}

void displayMedicines(void) {
    Medicine *t = medHead;
    if (!t) { printf("No medicines in list.\n"); return; }
    printf("\n--- Medicines ---\n");
    while (t) {
        printf("Batch: %s\n Brand: %s\n Generic: %s\n Manufacturer: %s\n Mfg: %s  Exp: %s\n Qty: %d  Status: %s\n\n",
               t->batchNumber, t->brandName, t->genericName, t->manufacturer,
               t->manufacturedDate, t->expiryDate, t->quantity, t->status);
        t = t->next;
    }
}

void updateMedicine(void) {
    char batch[32];
    printf("Batch to update: "); scanf("%31s", batch);
    Medicine *m = searchMedicine(batch);
    if (!m) { printf("Not found\n"); return; }
    printf("Current qty: %d\nNew qty: ", m->quantity); scanf("%d", &m->quantity);
    updateExpiryAndStockStatus(m);
    sortMedicinesByExpiry();
    saveMedicines();
    printf("Updated.\n");
}

void deleteMedicine(void) {
    char batch[32];
    printf("Batch to delete: "); scanf("%31s", batch);
    Medicine *t = medHead, *prev = NULL;
    while (t && strcmp(t->batchNumber, batch) != 0) { prev = t; t = t->next; }
    if (!t) { printf("Not found\n"); return; }
    if (!prev) medHead = t->next; else prev->next = t->next;
    free(t);
    saveMedicines();
    printf("Deleted.\n");
}

/* ---------- Stats ---------- */

void medicineStats(void) {
    updateAllMedicineStatuses();
    int total=0, expired=0, near=0, instock=0, low=0, out=0;
    Medicine *t = medHead;
    while (t) {
        total++;
        if (strcmp(t->status, "EXPIRED")==0) expired++;
        else if (strcmp(t->status, "NEAR EXPIRY")==0) near++;
        else if (strcmp(t->status, "OUT OF STOCK")==0) out++;
        else if (strcmp(t->status, "LOW STOCK")==0) low++;
        else if (strcmp(t->status, "IN STOCK")==0) instock++;
        else instock++;
        t = t->next;
    }
    printf("\n--- Statistics ---\n");
    printf("Total medicines : %d\n", total);
    printf("Expired         : %d\n", expired);
    printf("Near expiry     : %d\n", near);
    printf("In Stock        : %d\n", instock);
    printf("Low Stock       : %d\n", low);
    printf("Out of Stock    : %d\n", out);
    printf("------------------\n");
}

/* ---------- Menus ---------- */

void adminMenu(User *u) {
    (void)u;
    int ch;
    do {
        printf("\n--- Admin Menu ---\n");
        printf("1. Add medicine\n2. Show medicines\n3. Search medicine\n4. Update medicine qty\n5. Delete medicine\n6. Expiry tracker\n7. Statistics\n0. Logout\nChoice: ");
        if (scanf("%d", &ch) != 1) { while (getchar()!='\n'); ch = -1; }
        switch (ch) {
            case 1: addMedicine(); break;
            case 2: displayMedicines(); break;
            case 3: {
                char b[32]; printf("Batch to search: "); scanf("%31s", b);
                Medicine *m = searchMedicine(b);
                if (m) printf("Found: %s | %s | %s | Mfg:%s Exp:%s Qty:%d Status:%s\n",
                               m->batchNumber, m->brandName, m->genericName, m->manufacturedDate, m->expiryDate, m->quantity, m->status);
                else printf("Not found\n");
                break;
            }
            case 4: updateMedicine(); break;
            case 5: deleteMedicine(); break;
            case 6: checkExpiryStatus(); break;
            case 7: medicineStats(); break;
            case 0: break;
            default: printf("Invalid\n");
        }
    } while (ch != 0);
}

void staffMenu(User *u) {
    (void)u;
    int ch;
    do {
        printf("\n--- Staff Menu ---\n");
        printf("1. Show medicines\n2. Search medicine\n3. Expiry tracker\n4. Statistics\n0. Logout\nChoice: ");
        if (scanf("%d", &ch) != 1) { while (getchar()!='\n'); ch = -1; }
        switch (ch) {
            case 1: displayMedicines(); break;
            case 2: {
                char b[32]; printf("Batch to search: "); scanf("%31s", b);
                Medicine *m = searchMedicine(b);
                if (m) printf("Found: %s | %s | %s | Mfg:%s Exp:%s Qty:%d Status:%s\n",
                               m->batchNumber, m->brandName, m->genericName, m->manufacturedDate, m->expiryDate, m->quantity, m->status);
                else printf("Not found\n");
                break;
            }
            case 3: checkExpiryStatus(); break;
            case 4: medicineStats(); break;
            case 0: break;
            default: printf("Invalid\n");
        }
    } while (ch != 0);
}

/* ---------- main ---------- */

int main(void) {
    loadUsers();
    ensure_default_admin();
    loadMedicines();

    while (1) {
        int choice;
        printf("\n===== MAIN MENU =====\n");
        printf("1. Register (new user => STAFF)\n");
        printf("2. Login\n");
        printf("3. Exit\nChoice: ");
        if (scanf("%d", &choice) != 1) { while (getchar()!='\n'); continue; }
        if (choice == 1) {
            registerUser();
        } else if (choice == 2) {
            User *sessionUser = login_flow();
            if (!sessionUser) continue;
            /* decide which menu to show: show staff menu always, but if sessionUser->role==1 we can allow admin menu if they selected admin earlier (promotion already applied) */
            if (sessionUser->role == 1) adminMenu(sessionUser);
            else staffMenu(sessionUser);
        } else if (choice == 3) {
            printf("Exiting...\n");
            break;
        } else {
            printf("Invalid choice\n");
        }
    }
    return 0;
}