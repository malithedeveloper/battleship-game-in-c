/*
 Bu program, iki oyuncunun sırasıyla deniz üzerindeki gemileri vurmayı amaçladığı
 bir savaş gemisi oyununu temsil etmektedir. Oyunculardan biri biz, diğeri ise 
 bilgisayar rolündedir. Kod, gemilerin yerleştirilmesi, rastgele atış stratejileri 
 ve vuruş/miss durumlarının yönetilmesi gibi çeşitli fonksiyonlar içerir. 
 Bu oyunda temel olarak tablo gösterimi, gemi konumlama, karşılıklı atışlar, 
 geminin batma durumu, kazanma/kaybetme yazıları ve temel menü etkileşimlerini 
 kapsayan mantıklar yer almaktadır.
*/

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <windows.h>
#include <conio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

enum ANSI_Code {
    RESET, BOLD, UNDERLINE, MIDDLELINE, RED, LIGHTRED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, GRAY, DARKGRAY, LIGHTGREEN,
    BG_RED, BG_LIGHTRED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE, BG_GRAY, BG_LIGHTGREEN, ANSI_CODE_COUNT
};

const char *style[ANSI_CODE_COUNT] = {
    "\033[0m", "\033[1m", "\033[4m", "\033[9m", "\033[31m", "\033[91m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m",
    "\033[37m", "\033[90m", "\033[30m", "\033[92m", "\033[41m", "\033[101m", "\033[42m", "\033[43m", "\033[44m", "\033[45m",
    "\033[46m", "\033[47m", "\033[100m", "\033[102m"
};

typedef struct {
    char name[20];
    int length, hits, x, y, direction;
} Ship;

typedef struct {
    char name[20];
    int destroyed;
} ShipStatus;

Ship userShips[5], opponentShips[5];
int userShipCount = 0, opponentShipCount = 0;
int pointsUser = 0, pointsOpponent = 0;
static const char* opponentShipNames[] = {"Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"};

ShipStatus userShipStatus[5] = {
    {"Carrier", 0},
    {"Battleship", 0},
    {"Cruiser", 0},
    {"Submarine", 0},
    {"Destroyer", 0}
};

ShipStatus opponentShipStatus[5] = {
    {"Carrier", 0},
    {"Battleship", 0},
    {"Cruiser", 0},
    {"Submarine", 0},
    {"Destroyer", 0}
};

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point hits[17];
    int hitCount;
    int isVertical;  // -1: unknown, 0: horizontal, 1: vertical
    int lastDirection; // Last tried direction
} OpponentStrategy;

OpponentStrategy strategy = {{0}, 0, -1, -1};

/*
 Fonksiyon Bildirimleri (Prototipler)
 Her fonksiyonun ne yaptığına dair uzun Türkçe açıklamalar mevcuttur.
*/

// Ekranı temizlemek için kullanılan fonksiyonun deklarasyonu
// Ekrandaki tüm içeriği temizleyerek konsolu sıfırlar
void clearScreen();

// Dinamik olarak oluşturulan tablo hafızasını serbest bırakan fonksiyon
// 10 elemanlı pointer dizilerini ve kendisini free yapar
void tableFreeMem(int** table); 

// Verilen tabloyu sıfırlayan fonksiyon
// Tablodaki tüm hücre değerlerini 0 (varsayılan) yapar
void resetTable(int** table); 

// Kullanıcı girişi bufferını temizlemek için kullanılan fonksiyon
// _kbhit ile input varsa onları tüketerek temizlik yapar
void clearInputBuffer(); 

// Konsol penceresinin boyutlandırılmasını devre dışı bırakır
// Pencere stillerini değiştiren Windows API fonksiyonlarını kullanır
void disableResize(); 

// İmleci gizlemek için kullanılan fonksiyon
void hideCursor(); 

// Görsel tabloyu gösteren ve tablo içeriğine göre renklendirme yapan fonksiyon
// Tablodaki 0/1/2/3 değerlerine göre ekranda su, gemi, ıskalama ve vurma işaretleri basar
// show parametresi tabloyu gizli/görünür göstermeye yardımcı olur
void showTable(int **table, char* title, const char* color, int show, int point); 

// Kullanıcıdan klavye tuşlarıyla gemi konumu alarak tabloya gemi yerleştirmeyi sağlayan fonksiyon
// Belirtilen boyuttaki gemiyi yön (dikey/yatay) şeklinde uygun bir alana yerleştirir
void placeShip(int **table, int length, char *shipId); 

// Rakibin gemilerini rastgele yerleştiren fonksiyon
void randomPlaceShip(int **table, int *ships); 
// 5 gemiyi rastgele koordinatlara atar, etraflarındaki boşlukları kontrol eder

// Vuruş bilgilerini strateji dizisine ekleyen fonksiyon
// Rakibin gemisine vurduğumuz x,y koordinatını kaydeder
void addHitPoint(int x, int y); 

// Son kaydedilen vuruşu siler (geri alır) - stratejiye bağlı fonksiyon
// Vurulan noktalar dizisinden son eklenmiş olanı çıkarır
void removeLastHit(); 

// Rakibin son iki vuruşu yanyana mı, gemi yönde mi gibi fikir veren kontrol fonksiyonu
// Yatay veya dikey hizalama kontrolü yapar
int isAdjacentHit(int x, int y); 

// Geminin tamamen batıp batmadığını kontrol eden fonksiyon
int isShipDestroyed(int** table, Point hit, Ship* ships, int shipCount, ShipStatus* status); 

// Seçilen koordinata atış yapan fonksiyon (verdiği inputa göre 'hit' ya da 'miss' yapar)
// Kullanıcı veya bilgisayarın tablo üstüne atış gerçekleştirmesini sağlar
void destroyLocation(int **table, int isTurnAtUser); 

// Bilgisayarın rastgele veya stratejik hamleler yapmasını sağlayan fonksiyon
// Stratejiye bağlı olarak valid bir hedef seçer ve tabloyu günceller
void opponentTurn(int **userTable); 

// Oyunu başlatmadan önce, temel menü gösteren fonksiyon
// START ve INFORMATION seçenekleri içerir, klavyeden gelen yön tuşlarını alır
void showMainMenu(); 

// Kazanma durumunda ekrana özel metin yazar
void showWinAnimation(); 

// Kaybetme durumunda ekrana özel metin yazar.
void showLoseAnimation(); 

// Oyun karşı tarafın hazırlanmasını beklerken basit animasyon gösterir
void showPreparingAnimation(); 

// Oyun sıfırlama amaçlı skorlar gemi durum bilgileri gibi değişkenlerin ilk hallerine döner
void resetGameVariables(); 

int main() {
    SetConsoleOutputCP(CP_UTF8); // Konsolun türkçe karakter yazdırması için unicode çeşidini UTF-8 olarak ayarlar
    setlocale(LC_ALL, "Turkish_Turkey.1254");  // Türkçe karakterleri desteklemek için yerel ayarı ayarlar
    srand(time(NULL)); // Rastgele sayı üretmek için seed değerini zaman olarak ayarlar

    disableResize();
    hideCursor();

    resetGameVariables();

    system("cls");
    clearScreen(); // Konsol komudu kullanıp ekranı temizler

    // Oyun masalarını hazırlar
    int **userTable = (int**)malloc(10 * sizeof(int*));
    int **opponentTable = (int**)malloc(10 * sizeof(int*));

    for (int i = 0; i < 10; i++) { 
        userTable[i] = (int*)malloc(10 * sizeof(int));
        opponentTable[i] = (int*)malloc(10 * sizeof(int));
    }

    resetTable(userTable);
    resetTable(opponentTable);

    // Gemi yerleştirme işlemleri
    Ship userShips[5] = {
        {"Carrier", 5, 0, 0, 0, 0},
        {"Battleship", 4, 0, 0, 0, 0},
        {"Cruiser", 3, 0, 0, 0, 0},
        {"Submarine", 3, 0, 0, 0, 0},
        {"Destroyer", 2, 0, 0, 0, 0}
    };

    showMainMenu();

    for (int i = 0; i < 5; i++) {
        placeShip(userTable, userShips[i].length, userShips[i].name);
        system("cls");
    }

    showPreparingAnimation();

    randomPlaceShip(opponentTable, (int[]){5, 4, 3, 3, 2});

    for (int i = 0; i < 5; i++) {
        userShipStatus[i].destroyed = 0;
        opponentShipStatus[i].destroyed = 0;
    }

    int isTurnAtUser = 1;
    while (1) {
        if (isTurnAtUser) {
            showTable(opponentTable, "Opponent's board", style[BG_GREEN], 0, pointsUser);
            clearInputBuffer();
            destroyLocation(opponentTable, 1);
            if (pointsUser == 17) {
                system("cls");
                showWinAnimation();
                main();
                break;
            }
            isTurnAtUser = 0;
        } else {
            showTable(userTable, "Your board", style[BG_GREEN], 1, pointsOpponent);
            opponentTurn(userTable);
            if (pointsOpponent == 17) {
                system("cls");
                showLoseAnimation();
                main();
                break;
            }
            system("cls");
            isTurnAtUser = 1;
        }
    }

    // Bitiş: Table değişkenlerini memoryden sil
    tableFreeMem(userTable);
    tableFreeMem(opponentTable);
    return 0;
}
void clearScreen() { 
    // Ekranı temizleyip imleci sol üst köşeye alır
    gotoxy(0, 0); 
}
void tableFreeMem(int** table) { 
    // Dinamik olarak ayrılan tablo belleğini serbest bırakan fonksiyon
    for (int i = 0; i < 10; i++) free(table[i]); free(table); 
}
void resetTable(int** table) { 
    // Tabloyu varsayılan değerlere (0) döndürür
    for (int i = 0; i < 10; i++) for (int j = 0; j < 10; j++) table[i][j] = 0; 
}
void clearInputBuffer() { 
    // Klavye buffer'ını temizleyerek birikmiş tuş vuruşlarını yok sayar
    while (_kbhit()) _getch(); 
}
void disableResize() {
    // Konsol penceresinin boyutlandırılmasını engeller
    HWND consoleWindow = GetConsoleWindow();
    LONG style = GetWindowLong(consoleWindow, GWL_STYLE);
    style &= ~WS_SIZEBOX & ~WS_MAXIMIZEBOX;
    SetWindowLong(consoleWindow, GWL_STYLE, style);
    SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER );
}
void hideCursor() {
    // Konsol imlecini görünmez yapar
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;

    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = FALSE; // Hide cursor
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);
}
void showTable(int **table, char* title, const char* color, int show, int point) {
    // Tabloyu ekranda gösterip hücre değerlerine göre renklendirme yapar
    printf("%s%s%s", color, style[WHITE], style[BOLD]);
    for(int i = 0; i < (30 - strlen(title))/2; i++) printf(" ");
    printf("%s", title);
    for(int i = 0; i < (30 - strlen(title))/2 + (strlen(title) % 2 == 0 ? 0 : 1); i++) printf(" ");
    printf("%s\n", style[RESET]);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            switch (table[i][j]) {
                case 0: printf("%s%s W %s", style[BG_BLUE], style[WHITE], style[RESET]); break;
                case 1: printf("%s%s %c %s", show ? style[BG_RED] : style[BG_BLUE], style[WHITE], show ? 'S' : 'W', style[RESET]); break;
                case 2: printf("%s%s M %s", style[BG_GRAY], style[WHITE], style[RESET]); break;
                case 3: printf("%s%s H %s", style[BG_GREEN], style[WHITE], style[RESET]); break;
                default: printf("%d ", table[i][j]); break;
            }
        }
        printf("\n");
    }
    printf("\n");
}
void placeShip(int **table, int length, char *shipId) {
    // Kullanıcının yön tuşları ile gemi konumu belirlemesine olanak sağlar
    int x = 0, y = 0, errorExist = 0, errorCode = 0, direction = 0;
    char key;
    while (1) {
        clearScreen();
        printf("%s%s%s        Coords: (%d, %d)        %s", style[BG_GREEN], style[WHITE], style[BOLD], y+1, x+1, style[RESET]);
        gotoxy(31, 0);
        printf("                              \n");
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                if ((direction == 0 && i == x && j >= y && j < y + length) || (direction == 1 && j == y && i >= x && i < x + length)) {
                    printf("%s%s S %s", style[BG_YELLOW], style[WHITE], style[RESET]);
                } else {
                    switch (table[i][j]) {
                        case 0: printf("%s%s W %s", style[BG_BLUE], style[WHITE], style[RESET]); break;
                        case 1: printf("%s%s S %s", style[BG_RED], style[WHITE], style[RESET]); break;
                        default: printf("%d ", table[i][j]); break;
                    }
                }
            }
            switch (i) {
                case 1: case 3: printf("          %s%s                   ", style[RED], style[BG_WHITE]); for(int k = 0; k < strlen(shipId); k++) printf(" "); printf("%s", style[RESET]); break;
                case 2: printf("          %s%s   Place ship (%s)   %s", style[RED], style[BG_WHITE], shipId, style[RESET]); break;
                case 6: printf("         %s  ↑ ← ↓ →   to move%s", style[GREEN], style[RESET]); break;
                case 7: printf("         %s     R      to rotate%s", style[GREEN], style[RESET]); break;
                case 8: printf("         %s   ENTER    to place ship%s", style[GREEN], style[RESET]); break;
            }
            printf("\n");
        }
        if (errorExist) {
            errorExist = 0;
            gotoxy(0, 12);
            printf("%s                              %s\n", style[BG_GRAY], style[RESET]);
            switch (errorCode) {
                case 1: printf("%s%s%s   %sError: Invalid rotation.%s%s   %s\n", style[BG_GRAY], style[YELLOW], style[BOLD], style[UNDERLINE], style[RESET], style[BG_GRAY], style[RESET]); break;
                case 2: printf("%s%s%s     %sError: Ship adjacent%s%s     %s\n", style[BG_GRAY], style[YELLOW], style[BOLD], style[UNDERLINE], style[RESET], style[BG_GRAY], style[RESET]); break;
                default: printf("%s%s%s    %sError: No description.%s%s    %s\n", style[BG_GRAY], style[YELLOW], style[BOLD], style[UNDERLINE], style[RESET], style[BG_GRAY], style[RESET]); break;
            }
            printf("%s                              %s\n", style[BG_GRAY], style[RESET]);
        } else {
            gotoxy(0, 12);
            printf("%s                              %s\n%s                              %s\n%s                              %s\n", style[RESET], style[RESET], style[RESET], style[RESET], style[RESET], style[RESET]);
        }
        gotoxy(41, 3);
        key = _getch();
        if (key == 13) {
            int canPlace = 1;
            if (direction == 0) {
                for (int j = y; j < y + length; j++) {
                    if (table[x][j] != 0 || (x > 0 && table[x-1][j] != 0) || (x < 9 && table[x+1][j] != 0) || (j > 0 && table[x][j-1] != 0) || (j < 9 && table[x][j+1] != 0)) {
                        canPlace = 0;
                        errorCode = 2;
                        break;
                    }
                }
                if (canPlace) {
                    for (int j = y; j < y + length; j++) {
                        if ((j > 0 && table[x][j-1] == 1) || (j < 9 && table[x][j+1] == 1)) {
                            canPlace = 0;
                            errorCode = 2;
                            break;
                        }
                    }
                }
                if (canPlace) {
                    for (int j = y; j < y + length; j++) table[x][j] = 1;
                } else errorExist = 1;
            } else {
                for (int i = x; i < x + length; i++) {
                    if (table[i][y] != 0 || (y > 0 && table[i][y-1] != 0) || (y < 9 && table[i][y+1] != 0) || (i > 0 && table[i-1][y] != 0) || (i < 9 && table[i+1][y] != 0)) {
                        canPlace = 0;
                        errorCode = 2;
                        break;
                    }
                }
                if (canPlace) {
                    for (int i = x; i < x + length; i++) {
                        if ((i > 0 && table[i-1][y] == 1) || (i < 9 && table[i+1][y] == 1)) {
                            canPlace = 0;
                            errorCode = 2;
                            break;
                        }
                    }
                }
                if (canPlace) {
                    for (int i = x; i < x + length; i++) table[i][y] = 1;
                } else errorExist = 1;
            }
            if (canPlace) {
                strcpy(userShips[userShipCount].name, shipId);
                userShips[userShipCount].length = length;
                userShips[userShipCount].hits = 0;
                userShips[userShipCount].x = x;
                userShips[userShipCount].y = y;
                userShips[userShipCount].direction = direction;
                userShipCount++;
                break;
            }
        } else if (key == 'r' || key == 'R') {
            if ((9-x < length-1 && direction == 0) || (9-y < length-1 && direction == 1)) {
                errorExist = 1;
                errorCode = 1;
            } else direction = !direction;
        } else if (key == 72 && x > 0) x--;
        else if (key == 80 && x < 9 - (direction ? length - 1 : 0)) x++;
        else if (key == 75 && y > 0) y--;
        else if (key == 77 && y < 9 - (direction ? 0 : length - 1)) y++;
    }
    clearScreen();
}
void randomPlaceShip(int **table, int *ships) {
    // Gemileri rastgele koordinatlara yerleştirir
    for (int i = 0; i < 5; i++) {
        int x = rand() % 10, y = rand() % 10, direction = rand() % 2, length = ships[i], canPlace = 1;
        if (direction == 0) {
            if (y + length > 10) y = 10 - length;
            for (int j = y; j < y + length; j++) {
                if (table[x][j] != 0 ||
                    (x > 0 && table[x-1][j] != 0) ||
                    (x < 9 && table[x+1][j] != 0) ||
                    (j > 0 && table[x][j-1] != 0) ||
                    (j < 9 && table[x][j+1] != 0)) {
                    canPlace = 0;
                    break;
                }
            }
            if (canPlace) {
                for (int j = y; j < y + length; j++) {
                    table[x][j] = 1;
                }
                strcpy(opponentShips[opponentShipCount].name, opponentShipNames[i]);
                opponentShips[opponentShipCount].length = length;
                opponentShips[opponentShipCount].hits = 0;
                opponentShips[opponentShipCount].x = x;
                opponentShips[opponentShipCount].y = y;
                opponentShips[opponentShipCount].direction = direction;
                opponentShipCount++;
            } else {
                i--;
            }
        } else {
            if (x + length > 10) x = 10 - length;
            for (int j = x; j < x + length; j++) {
                if (table[j][y] != 0 ||
                    (y > 0 && table[j][y-1] != 0) ||
                    (y < 9 && table[j][y+1] != 0) ||
                    (j > 0 && table[j-1][y] != 0) ||
                    (j < 9 && table[j+1][y] != 0)) {
                    canPlace = 0;
                    break;
                }
            }
            if (canPlace) {
                for (int j = x; j < x + length; j++) {
                    table[j][y] = 1;
                }
                strcpy(opponentShips[opponentShipCount].name, opponentShipNames[i]);
                opponentShips[opponentShipCount].length = length;
                opponentShips[opponentShipCount].hits = 0;
                opponentShips[opponentShipCount].x = x;
                opponentShips[opponentShipCount].y = y;
                opponentShips[opponentShipCount].direction = direction;
                opponentShipCount++;
            } else {
                i--;
            }
        }
    }
}
void addHitPoint(int x, int y) {
    // Vurulan x,y koordinatını strateji dizisine ekler
    if (strategy.hitCount < 17) {
        strategy.hits[strategy.hitCount].x = x;
        strategy.hits[strategy.hitCount].y = y;
        strategy.hitCount++;
    }
}
void removeLastHit() {
    // Kaydedilen son vuruşu strateji dizisinden temizler
    if (strategy.hitCount > 0) {
        strategy.hitCount--;
    }
}
Point getLastHit() {
    // Kaydedilen son vuruş koordinatını döndürür
    if (strategy.hitCount > 0) {
        return strategy.hits[strategy.hitCount - 1];
    }
    return (Point){-1, -1};
}
int isAdjacentHit(int x, int y) {
    // Son iki vuruşun yan yana olup olmadığını kontrol eder
    if (strategy.hitCount < 2) return 0;
    
    Point last = strategy.hits[strategy.hitCount - 1];
    Point prev = strategy.hits[strategy.hitCount - 2];
    
    if (last.x == prev.x) { // Dikey ise
        strategy.isVertical = 1;
        return (x == last.x && (abs(y - last.y) == 1 || abs(y - prev.y) == 1));
    } else if (last.y == prev.y) { // Yatay ise
        strategy.isVertical = 0;
        return (y == last.y && (abs(x - last.x) == 1 || abs(x - prev.x) == 1));
    }
    return 0;
}
int isShipDestroyed(int** table, Point hit, Ship* ships, int shipCount, ShipStatus* status) {
    // Geminin isabet alıp tamamen batıp batmadığını kontrol eder
    for (int i = 0; i < shipCount; i++) {
        Ship* ship = &ships[i];
        int hitCount = 0;
        
        if (ship->direction == 0) { // Yatay ise
            if (hit.x == ship->x && hit.y >= ship->y && hit.y < ship->y + ship->length) {
                for (int j = ship->y; j < ship->y + ship->length; j++) {
                    if (table[ship->x][j] == 3) hitCount++;
                }
            }
        } else { // Dikey ise
            if (hit.y == ship->y && hit.x >= ship->x && hit.x < ship->x + ship->length) {
                for (int j = ship->x; j < ship->x + ship->length; j++) {
                    if (table[j][ship->y] == 3) hitCount++;
                }
            }
        }
        
        if (hitCount == ship->length && !status[i].destroyed) {
            status[i].destroyed = 1;
            return i;
        }
    }
    return -1;
}
void updateShipStatusDisplay(int isOpponent) {
    // Batırılan gemileri ekranda günceller
    ShipStatus* status = isOpponent ? opponentShipStatus : userShipStatus;
    gotoxy(70, 6);
    printf("    %s%sCarrier%s         %s%sBattleship%s", 
        status[0].destroyed ? style[GRAY] : style[RED],
        status[0].destroyed ? style[MIDDLELINE] : "",
        style[RESET],
        status[1].destroyed ? style[GRAY] : style[RED],
        status[1].destroyed ? style[MIDDLELINE] : "",
        style[RESET]);
    
    gotoxy(70, 7);
    printf("    %s%sCruiser%s         %s%sSubmarine%s", 
        status[2].destroyed ? style[GRAY] : style[RED],
        status[2].destroyed ? style[MIDDLELINE] : "",
        style[RESET],
        status[3].destroyed ? style[GRAY] : style[RED],
        status[3].destroyed ? style[MIDDLELINE] : "",
        style[RESET]);
    
    gotoxy(70, 8);
    printf("           %s%sDestroyer%s", 
        status[4].destroyed ? style[GRAY] : style[RED],
        status[4].destroyed ? style[MIDDLELINE] : "",
        style[RESET]);
}
void markAroundDestroyedShip(int **table, Ship ship) {
    // Batmış gemi çevresini 'miss' ile işaretler
    if (ship.direction == 0) { // Yatay
        // Geminin üstünü ve altını işaretle
        if (ship.x > 0) {
            for (int j = ship.y; j < ship.y + ship.length; j++) {
                if (table[ship.x-1][j] != 3) table[ship.x-1][j] = 2;
            }
        }
        if (ship.x < 9) {
            for (int j = ship.y; j < ship.y + ship.length; j++) {
                if (table[ship.x+1][j] != 3) table[ship.x+1][j] = 2;
            }
        }
        // Geminin sağı ve solunu işaretle
        if (ship.y > 0 && table[ship.x][ship.y-1] != 3) {
            table[ship.x][ship.y-1] = 2;
        }
        if (ship.y + ship.length < 10 && table[ship.x][ship.y+ship.length] != 3) {
            table[ship.x][ship.y+ship.length] = 2;
        }
    } else { // Dikey
        // Geminin sağı ve solunu işaretle
        if (ship.y > 0) {
            for (int i = ship.x; i < ship.x + ship.length; i++) {
                if (table[i][ship.y-1] != 3) table[i][ship.y-1] = 2;
            }
        }
        if (ship.y < 9) {
            for (int i = ship.x; i < ship.x + ship.length; i++) {
                if (table[i][ship.y+1] != 3) table[i][ship.y+1] = 2;
            }
        }
        // Geminin üstünü ve altını işaretle
        if (ship.x > 0 && table[ship.x-1][ship.y] != 3) {
            table[ship.x-1][ship.y] = 2;
        }
        if (ship.x + ship.length < 10 && table[ship.x+ship.length][ship.y] != 3) {
            table[ship.x+ship.length][ship.y] = 2;
        }
    }
}
int isValidTarget(int** table, int x, int y) {
    // Girilen x,y hücresinin atış için uygun olup olmadığını kontrol eder
    // O kordinat denendiğine bakar, eğer 1'den büyükse zaten vurulmuş demektir
    if (table[x][y] > 1) return 0;
    
    // Etrafındaki hücrelere bak (yukarı, aşağı, sol, sağ, çapraz)
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    
    // Erişilebilir ve bloke edilmiş hücreleri say
    int surroundedCount = 0;
    int accessibleCells = 0;
    
    // Bütün yönlere bakarak etrafı kontrol et
    for (int i = 0; i < 8; i++) {
        int newX = x + dx[i];
        int newY = y + dy[i];
        
        // Eğer hücre dışında ise atla
        if (newX < 0 || newX >= 10 || newY < 0 || newY >= 10) {
            continue;
        }
        
        accessibleCells++;
        if (table[newX][newY] == 2 || table[newX][newY] == 3) {
            surroundedCount++;
        }
    }
    
    // Eğer tüm erişilebilir hücreler vurulmuş/ıskalanmış değilse konum geçerlidir
    return surroundedCount < accessibleCells;
}
void destroyLocation(int **table, int isTurnAtUser) {
    // Belirlenen konuma atış yapıp vuruş/ıskalama işlemlerini yönetir
    int x = 0, y = 0, errorExist = 0, errorCode = 0;
    char key;
    while (1) {
        clearScreen();
        if (isTurnAtUser) showTable(table, "Opponent's board", style[BG_LIGHTRED], 0, 0);
        else showTable(table, "Your board", style[BG_GREEN], 1, 0);
        gotoxy(0, 2);
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                if (i == y && j == x) {
                    gotoxy((j*3)+1, i + 2);
                    printf("%s%s X %s", style[BG_YELLOW], style[WHITE], style[RESET]);
                }
            }
            printf("\n");
        }
        gotoxy(31, 2);
        printf("       %s%s                              %s  ", style[WHITE], style[BG_LIGHTGREEN], style[RESET]);
        gotoxy(31, 3);
        printf("       %s%s%s           Your turn          %s  ", style[WHITE], style[BG_LIGHTGREEN], style[BOLD], style[RESET]);
        gotoxy(31, 4);
        printf("       %s%s                              %s  ", style[WHITE], style[BG_LIGHTGREEN], style[RESET]);
        gotoxy(31, 6);
        printf("         %s%s  %sDESTROY LOCATION%s %s(%d, %d)%s  ", style[GREEN], style[BOLD], style[UNDERLINE], style[RESET], style[WHITE], x+1, y+1, style[RESET]);
        gotoxy(31, 7);
        printf("         %s                              %s", style[GREEN], style[RESET]);
        gotoxy(31, 8);
        printf("         %s  ↑ ← ↓ →   to move          %s", style[GREEN], style[RESET]);
        gotoxy(31, 9);
        printf("         %s   ENTER    to destroy%s", style[GREEN], style[RESET]);
        gotoxy(70, 2);
        printf("  %s%s                              %s", style[RED], style[BG_WHITE], style[RESET]);
        gotoxy(70, 3);
        printf("  %s%s%s       Ship parts %c%d/17       %s", style[DARKGRAY], style[BG_WHITE], style[BOLD], pointsUser <= 9 ? '0' : '\0', pointsUser, style[RESET]);
        gotoxy(70, 4);
        printf("  %s%s                              %s", style[RED], style[BG_WHITE], style[RESET]);
        updateShipStatusDisplay(1);
        if (errorExist) {
            errorExist = 0;
            gotoxy(0, 12);
            printf("%s                              %s\n", style[BG_GRAY], style[RESET]);
            switch (errorCode) {
                case 1: printf("%s%s%s     %sError: Already tried%s%s     %s\n", style[BG_GRAY], style[YELLOW], style[BOLD], style[UNDERLINE], style[RESET], style[BG_GRAY], style[RESET]); break;
                default: printf("%s%s%s    %sError: No description.%s%s    %s\n", style[BG_GRAY], style[YELLOW], style[BOLD], style[UNDERLINE], style[RESET], style[BG_GRAY], style[RESET]); break;
            }
            printf("%s                              %s\n", style[BG_GRAY], style[RESET]);
        } else {
            // Hata mesajını sil
            gotoxy(0, 12);
            printf("%s                              %s\n%s                              %s\n%s                              %s\n", style[RESET], style[RESET], style[RESET], style[RESET], style[RESET], style[RESET]);
        }

        key = _getch();
        if (key == 13) { // Enter tuşuna basarak atış yap
            if (table[y][x] == 1) {
                table[y][x] = 3; // Vuruldu
                pointsUser++;
                
                // Geminin yıkılıp yıkılmadığını kontrol et
                gotoxy(0, 0);
                if (isTurnAtUser) {
                    showTable(table, "Opponent's board", style[BG_LIGHTRED], 0, 0);
                } else {
                    showTable(table, "Your board", style[BG_GREEN], 1, 0);
                }
                gotoxy(31, 2);
                printf("       %s%s                              %s", style[RED], style[BG_GREEN], style[RESET]);
                gotoxy(31, 3);
                printf("       %s%s%s             HIT!             %s", style[WHITE], style[BOLD], style[BG_GREEN], style[RESET]);
                gotoxy(31, 4);
                printf("       %s%s                              %s", style[RED], style[BG_GREEN], style[RESET]);
                gotoxy(31, 7);
                printf("         %s                              %s", style[GREEN], style[RESET]);
                gotoxy(31, 7);
                printf("                                           ");
                gotoxy(31, 8);
                printf("         %s                              %s", style[GREEN], style[RESET]);
                gotoxy(31, 9);
                printf("         %s                            %s", style[GREEN], style[RESET]);
                gotoxy(31, 10);
                printf("         %s                            %s", style[GREEN], style[RESET]);
                gotoxy(31, 6);
                printf("         %s                            %s", style[GREEN], style[RESET]);
                gotoxy(70, 2);
                printf("  %s%s                              %s", style[RED], style[BG_WHITE], style[RESET]);
                gotoxy(70, 3);
                printf("  %s%s%s    Ship parts %c%d/17 (+1)    %s", style[DARKGRAY], style[BG_WHITE], style[BOLD], pointsUser <= 9 ? '0' : '\0', pointsUser, style[RESET]);
                gotoxy(70, 4);
                printf("  %s%s                              %s", style[RED], style[BG_WHITE], style[RESET]);
                updateShipStatusDisplay(1);
                Point hitPoint = {y, x};
                int destroyedShipIndex = isShipDestroyed(table, hitPoint, opponentShips, opponentShipCount, opponentShipStatus);
                if (destroyedShipIndex != -1) {
                    updateShipStatusDisplay(1);
                    markAroundDestroyedShip(table, opponentShips[destroyedShipIndex]);
                    gotoxy(31, 7);
                    printf("          %s%s%sSHIP DESTROYED (%s)%s ", style[BG_RED], style[BOLD], style[YELLOW], opponentShips[destroyedShipIndex].name, style[RESET]);
                    Sleep(450);
                    gotoxy(31, 7);
                    printf("                                         ");
                    Sleep(450);
                    gotoxy(31, 7);
                    printf("          %s%s%sSHIP DESTROYED (%s)%s ", style[BG_RED], style[BOLD], style[YELLOW], opponentShips[destroyedShipIndex].name, style[RESET]);
                    Sleep(450);
                    gotoxy(31, 7);
                    printf("                                         ");
                    Sleep(450);
                    gotoxy(31, 7);
                    printf("          %s%s%sSHIP DESTROYED (%s)%s ", style[BG_RED], style[BOLD], style[YELLOW], opponentShips[destroyedShipIndex].name, style[RESET]);
                }
                gotoxy(0, 15);
                Sleep(1000); 
                clearInputBuffer();
                if (pointsUser == 17) {
                    break;
                } else {
                    destroyLocation(table, isTurnAtUser);
                }
            } else if (table[y][x] == 3 || table[y][x] == 2) {
                errorExist = 1;
                errorCode = 1;
            } else {
                table[y][x] = 2; // Kaçırıldı
                gotoxy(0, 0);
                if (isTurnAtUser) {
                    showTable(table, "Opponent's board", style[BG_LIGHTRED], 0, 0);
                } else {
                    showTable(table, "Your board", style[BG_GREEN], 1, 0);
                }
                gotoxy(31, 2);
                printf("       %s%s                              %s", style[RED], style[BG_GRAY], style[RESET]);
                gotoxy(31, 3);
                printf("       %s%s%s             MISS             %s", style[WHITE], style[BOLD], style[BG_GRAY], style[RESET]);
                gotoxy(31, 4);
                printf("       %s%s                              %s", style[RED], style[BG_GRAY], style[RESET]);
                gotoxy(31, 7);
                printf("         %s                     %s", style[GREEN], style[RESET]);
                gotoxy(31, 8);
                printf("         %s                            %s", style[GREEN], style[RESET]);
                gotoxy(31, 9);
                printf("         %s                            %s", style[GREEN], style[RESET]);
                gotoxy(31, 10);
                printf("         %s                            %s", style[GREEN], style[RESET]);
                gotoxy(31, 6);
                printf("         %s                            %s", style[GREEN], style[RESET]);
                gotoxy(0, 15);
                Sleep(1000); 
                break; // Sıra rakibe geçti
            }
            if (!errorExist) {
                break;
            }
        } else if (key == 72 && y > 0) y--;
        else if (key == 80 && y < 9) y++;
        else if (key == 75 && x > 0) x--;
        else if (key == 77 && x < 9) x++;
    }
    gotoxy(31, 2);
    printf("                                     ");
    gotoxy(31, 3);
    printf("                                     ");
    gotoxy(31, 4);
    printf("                                     ");
    gotoxy(31, 7);
    printf("                                     ");
    gotoxy(31, 8);
    printf("                                     ");
    clearScreen();
}
void opponentTurn(int **userTable) {
    // Bilgisayarın rastgele veya stratejik hamle yapmasını sağlar
    Point target = {-1, -1};
    int pathFound = 0;

    if (strategy.hitCount > 0) {
        Point last = getLastHit();
        int directions[4][2] = {{0,1}, {0,-1}, {1,0}, {-1,0}};
        
        // İkinci hit'ten sonra yön tespiti
        if (strategy.hitCount >= 2) {
            Point prevHit = strategy.hits[strategy.hitCount - 2];
            
            // Yatay mı dikey mi kontrol et
            if (last.x == prevHit.x) {  // Yatay hizalama
                strategy.isVertical = 0;
                // Sadece yatay yönleri dene (ilk iki direction)
                for (int i = 0; i < 2; i++) {
                    int newY = last.y + directions[i][1];
                    if (newY >= 0 && newY < 10 && 
                        (userTable[last.x][newY] == 0 || userTable[last.x][newY] == 1)) {
                        target.x = last.x;
                        target.y = newY;
                        pathFound = 1;
                        break;
                    }
                }
            } else if (last.y == prevHit.y) {  // Dikey hizalama
                strategy.isVertical = 1;
                // Sadece dikey yönleri dene (son iki direction)
                for (int i = 2; i < 4; i++) {
                    int newX = last.x + directions[i][0];
                    if (newX >= 0 && newX < 10 && 
                        (userTable[newX][last.y] == 0 || userTable[newX][last.y] == 1)) {
                        target.x = newX;
                        target.y = last.y;
                        pathFound = 1;
                        break;
                    }
                }
            }
        } else {
            // İlk hit sonrası tüm yönleri dene
            for (int i = 0; i < 4; i++) {
                int newX = last.x + directions[i][0];
                int newY = last.y + directions[i][1];
                
                if (newX >= 0 && newX < 10 && newY >= 0 && newY < 10 &&
                    (userTable[newX][newY] == 0 || userTable[newX][newY] == 1)) {
                    target.x = newX;
                    target.y = newY;
                    pathFound = 1;
                    break;
                }
            }
        }

        // Eğer devam edilecek yön bulunamadıysa
        if (!pathFound) {
            removeLastHit();
            if (strategy.hitCount > 0) {
                opponentTurn(userTable);
                return;
            } else {
                // Hit listesi boşaldı, stratejiyi sıfırla
                strategy.isVertical = -1;
                strategy.hitCount = 0;
            }
        }
    }

    // Hit'e göre hareket bulunamadıysa rastgele hamle yap
    if (!pathFound) {
        int attempts = 0;
        do {
            target.x = rand() % 10;
            target.y = rand() % 10;
            attempts++;
            
            // Geçerli target yoksa sonsuz döngüyü engelle
            if (attempts > 100) {
                // İlk bulunan bölgeye ateş et
                for(int i = 0; i < 10; i++) {
                    for(int j = 0; j < 10; j++) {
                        if (userTable[i][j] <= 1) {
                            target.x = i;
                            target.y = j;
                            goto targetFound;
                        }
                    }
                }
                targetFound:
                break;
            }
        } while (!isValidTarget(userTable, target.x, target.y) || userTable[target.x][target.y] > 1);
    }
    int x = 0, y = 0, firstStart = 0;
    while (x != target.x || y != target.y) {
        clearScreen();
        showTable(userTable, "Your board", style[BG_LIGHTGREEN], 1, 0);
        gotoxy((y * 3) + 1, x + 2);
        printf("%s%s X %s", style[BG_YELLOW], style[WHITE], style[RESET]);
        gotoxy(31, 2);
        printf("       %s%s                              %s", style[WHITE], style[BG_RED], style[RESET]);
        gotoxy(31, 3);
        printf("       %s%s%s        Opponent's turn       %s", style[WHITE], style[BG_RED], style[BOLD], style[RESET]);
        gotoxy(31, 4);
        printf("       %s%s                              %s", style[WHITE], style[BG_RED], style[RESET]);
        gotoxy(31, 6);
        printf("         %s%s Opponent thinking %s%s(%d, %d)  %s  ", style[LIGHTRED], style[BOLD], style[RESET], style[BOLD], y+1, x+1, style[RESET]);
        gotoxy(31, 7);
        printf("         %s                              %s", style[GREEN], style[RESET]);
        updateShipStatusDisplay(0);
        gotoxy(0, 15);
        int direction = rand() % 2, wrongMoveChance = rand() % 10;
        if (wrongMoveChance < 2) {
            if (direction == 0) {
                if (rand() % 2 == 0 && x > 0) x--;
                else if (x < 9) x++;
            } else {
                if (rand() % 2 == 0 && y > 0) y--;
                else if (y < 9) y++;
            }
            Sleep(10 + rand() % 41);
        } else {
            if (direction == 0) {
                if (x < target.x && x < 9) x++;
                else if (x > target.x && x > 0) x--;
            } else {
                if (y < target.y && y < 9) y++;
                else if (y > target.y && y > 0) y--;
            }
        }
        gotoxy(70, 2);
        printf("  %s%s                              %s   ", style[RED], style[BG_WHITE], style[RESET]);
        gotoxy(70, 3);
        printf("  %s%s%s       Ship parts %c%d/17       %s   ", style[DARKGRAY], style[BG_WHITE], style[BOLD], pointsOpponent <= 9 ? '0' : '\0', pointsOpponent, style[RESET]);
        gotoxy(70, 4);
        printf("  %s%s                              %s   ", style[RED], style[BG_WHITE], style[RESET]);
        if (!firstStart) {
            Sleep(250 + rand() % 500); 
            firstStart = 1;
        } else {
            Sleep(10 + rand() % 241);
        }  
    }
    clearScreen();
    showTable(userTable, "Your board", style[BG_LIGHTGREEN], 1, 0);
    gotoxy((y * 3) + 1, x + 2);
    printf("%s%s X %s", style[BG_YELLOW], style[WHITE], style[RESET]);
    gotoxy(31, 2);
    printf("       %s%s                              %s", style[WHITE], style[BG_RED], style[RESET]);
    gotoxy(31, 3);
    printf("       %s%s%s        Opponent's turn       %s", style[WHITE], style[BG_RED], style[BOLD], style[RESET]);
    gotoxy(31, 4);
    printf("       %s%s                              %s", style[WHITE], style[BG_RED], style[RESET]);
    gotoxy(31, 6);
    printf("         %s%s Opponent thinking %s%s(%d, %d)  %s  ", style[LIGHTRED], style[BOLD], style[RESET], style[BOLD], y+1, x+1, style[RESET]);
    gotoxy(31, 7);
    printf("         %s                              %s", style[GREEN], style[RESET]);
    gotoxy(0, 15);
    Sleep(250 + rand() % 500);
    if (userTable[target.x][target.y] == 1) {
        userTable[target.x][target.y] = 3;
        addHitPoint(target.x, target.y);
        pointsOpponent++;
        
        Point hitPoint = {target.x, target.y};
        gotoxy(0, 0);
        showTable(userTable, "Your board", style[BG_LIGHTGREEN], 1, 0);
        gotoxy(31, 2);
        printf("       %s%s                              %s", style[RED], style[BG_GREEN], style[RESET]);
        gotoxy(31, 3);
        printf("       %s%s%s             HIT!             %s", style[WHITE], style[BOLD], style[BG_GREEN], style[RESET]);
        gotoxy(31, 4);
        printf("       %s%s                              %s", style[RED], style[BG_GREEN], style[RESET]);
        gotoxy(31, 7);
        printf("         %s                              %s", style[GREEN], style[RESET]);
        gotoxy(31, 7);
        printf("         %s%s   ship part at %s%s(%d, %d)  %s  ", style[GREEN], style[BOLD], style[RESET], style[BOLD], y+1, x+1, style[RESET]);
        gotoxy(31, 8);
        printf("         %s                            %s", style[GREEN], style[RESET]);
        gotoxy(31, 10);
        printf("         %s                            %s", style[GREEN], style[RESET]);
        gotoxy(31, 6);
        printf("         %s%s Opponent destroyed your %s ", style[GREEN], style[BOLD], style[RESET]);
        gotoxy(70, 2);
        printf("  %s%s                              %s   ", style[RED], style[BG_WHITE], style[RESET]);
        gotoxy(70, 3);
        printf("  %s%s%s    Ship parts %c%d/17 (+1)     %s   ", style[DARKGRAY], style[BG_WHITE], style[BOLD], pointsOpponent <= 9 ? '0' : '\0', pointsOpponent, style[RESET]);
        gotoxy(70, 4);
        printf("  %s%s                              %s   ", style[RED], style[BG_WHITE], style[RESET]);
        int destroyedShipIndex = isShipDestroyed(userTable, hitPoint, userShips, userShipCount, userShipStatus);
        
        if (destroyedShipIndex != -1) {
            strategy.hitCount = 0;
            strategy.isVertical = -1;
            updateShipStatusDisplay(0);
            markAroundDestroyedShip(userTable, userShips[destroyedShipIndex]);
            gotoxy(31, 9);
            printf("          %s%s%sSHIP DESTROYED (%s)%s ", style[BG_RED], style[BOLD], style[YELLOW], userShips[destroyedShipIndex], style[RESET]);
            Sleep(450);
            gotoxy(31, 9);
            printf("                                      ");
            Sleep(450);
            gotoxy(31, 9);
            printf("          %s%s%sSHIP DESTROYED (%s)%s ", style[BG_RED], style[BOLD], style[YELLOW], userShips[destroyedShipIndex], style[RESET]);
            Sleep(450);
            gotoxy(31, 9);
            printf("                                      ");
            Sleep(450);
            gotoxy(31, 9);
            printf("          %s%s%sSHIP DESTROYED (%s)%s ", style[BG_RED], style[BOLD], style[YELLOW], userShips[destroyedShipIndex], style[RESET]);
        }
        Sleep(1000);
        gotoxy(31, 9);
        printf("                                      ");
        if (pointsOpponent != 17) {
            opponentTurn(userTable);  
        }
    } else {
        userTable[target.x][target.y] = 2;
        gotoxy(0, 0);
        showTable(userTable, "Your board", style[BG_LIGHTGREEN], 1, 0);
        gotoxy(31, 2);
        printf("       %s%s                              %s", style[RED], style[BG_GRAY], style[RESET]);
        gotoxy(31, 3);
        printf("       %s%s%s             MISS             %s", style[WHITE], style[BOLD], style[BG_GRAY], style[RESET]);
        gotoxy(31, 4);
        printf("       %s%s                              %s", style[RED], style[BG_GRAY], style[RESET]);
        gotoxy(31, 6);
        printf("         %s%s Opponent missed at %s%s(%d, %d)     %s  ", style[GRAY], style[BOLD], style[RESET], style[WHITE], y+1, x+1, style[RESET]);
        gotoxy(31, 7);
        printf("         %s                     %s", style[GREEN], style[RESET]);
        gotoxy(31, 8);
        printf("         %s                            %s", style[GREEN], style[RESET]);
        gotoxy(31, 9);
        printf("         %s                            %s", style[GREEN], style[RESET]);
        gotoxy(31, 10);
        printf("         %s                            %s", style[GREEN], style[RESET]);
        Sleep(1000);
    }
}
void showMainMenu() {
    // Oyunu başlatmadan önce ana menüyü gösterir
    int selection = 0; // 0: START, 1: CREDITS
    while (1) {
        clearScreen();
        printf("%s%s", style[BOLD], style[YELLOW]);
        printf("╔════════════════════════════════════════════════════════════════╗\n");
        printf("║ %s%s               Battleship Game - (Amiral Battı)               %s%s%s ║\n",style[BG_RED],style[WHITE],style[RESET], style[BOLD], style[YELLOW]);
        printf("║                  ~.                                            ║\n");
        printf("║          Ya...___|..aab     .   .        %s%s%s┌─────────────────┐%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║           Y88a  Y88o  Y88a   (     )     %s%s%s│                 │%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║            Y88b  Y88b  Y88b   `.oo'      %s%s%s│    [ START ]    │%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║            :888  :888  :888  ( (`-'      %s%s%s│                 │%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║   .---.    d88P  d88P  d88P   `.`.       %s%s%s├─────────────────┤%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║  / .-._)  d8P'\"\"\"|\"\"\"-Y8P       `.`.     %s%s%s│                 │%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║ ( (`._) .-.  .-. |.-.  .-.  .-.   ) )    %s%s%s│ [ INFORMATION ] │%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║  \\ `---( O )( O )( O )( O )( O )-' /     %s%s%s│                 │%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║   `.    `-'  `-'  `-'  `-'  `-'  .'      %s%s%s└─────────────────┘%s%s%s   ║\n", style[RESET], style[BOLD], style[WHITE], style[RESET], style[YELLOW], style[BOLD]);
        printf("║  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                          ║\n");
        printf("╚════════════════════════════════════════════════════════════════╝\n");
        printf("                                                                          \n"                                                                          );
        // Seçili butonu vurgula
        gotoxy(49, 6);
        if (selection == 0) {
            printf("%s%s%s[ START ]%s", style[BOLD], style[BG_RED], style[YELLOW], style[RESET]);
        }
        gotoxy(46, 10);
        if (selection == 1) {
            printf("%s%s%s[ INFORMATION ]%s", style[BOLD], style[BG_RED], style[YELLOW], style[RESET]);
        }

        char key = _getch();
        if (key == 72 || key == 75) selection = 0; // Yukarı veya sol
        else if (key == 80 || key == 77) selection = 1; // Aşağı veya sağ
        else if (key == 13) {
            if (selection == 0) {
                clearScreen();
                system("cls");
                printf("%s%s\n", style[BOLD], style[GREEN]);
                printf("╔═══════════════════════════════════════════════════════╗\n");
                printf("║                                                       ║\n");
                printf("║                                                       ║\n");
                printf("║                  Preparing the game.                  ║\n");
                printf("║                                                       ║\n");
                printf("║                                                       ║\n");
                printf("╚═══════════════════════════════════════════════════════╝\n");
                printf("%s%s\n", style[RESET], style[GREEN]);
                Sleep(1000);
                clearInputBuffer();
                system("cls");
                break;
            } else {
                clearScreen(); 
                system("cls");
                printf("%s%s\n", style[BOLD], style[BLUE]);
                printf("╔═══════════════════════════════════════════════════════╗\n");
                printf("║                                                       ║\n");
                printf("║                                                       ║\n");
                printf("║          made by malithedeveloper for BSM151          ║\n");
                printf("║                                                       ║\n");
                printf("║                                                       ║\n");
                printf("╚═══════════════════════════════════════════════════════╝\n");
                printf("%s%s\n", style[RESET], style[WHITE]);
                printf("                   ENTER to return...");
                while (_getch() != 13) {}
                clearInputBuffer();
            }
        }
    }
}
void showWinAnimation() {
    // Kazanma durumunda ekrana özel yazılar ve efektler yansıtır
    clearScreen();
    printf("%s%s", style[BOLD], style[GREEN]);
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ %s%s                      THE FIGHT IS OURS!                      %s%s%s ║\n",style[BG_LIGHTGREEN],style[WHITE],style[RESET], style[BOLD], style[GREEN]);
    printf("║                              ~.                                ║\n");
    printf("║                      Ya...___|..aab     .   .                  ║\n");
    printf("║                       Y88a  Y88o  Y88a   (     )               ║\n");
    printf("║                        Y88b  Y88b  Y88b   `.oo'                ║\n");
    printf("║                        :888  :888  :888  ( (`-'                ║\n");
    printf("║               .---.    d88P  d88P  d88P   `.`.                 ║\n");
    printf("║              / .-._)  d8P'\"\"\"|\"\"\"-Y8P       `.`.               ║\n");
    printf("║             ( (`._) .-.  .-. |.-.  .-.  .-.   ) )              ║\n");
    printf("║              \\ `---( O )( O )( O )( O )( O )-' /               ║\n");
    printf("║               `.    `-'  `-'  `-'  `-'  `-'  .'                ║\n");
    printf("║              ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~              ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("%s\n                        ENTER to return...", style[RESET]);
    while (_getch() != 13) {}
    clearInputBuffer();
}
void showLoseAnimation() {
    // Kaybetme durumunda ekrana özel yazılar gönderir
    clearScreen();
    printf("%s%s\n", style[BOLD], style[RED]);
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║ %s%s            MISSION FAILED! WE'LL GET EM NEXT TIME            %s%s%s ║\n",style[BG_LIGHTRED],style[WHITE],style[RESET], style[BOLD], style[RED]);
    printf("║                              ~.                                ║\n");
    printf("║                      Ya...___|..aab     .   .                  ║\n");
    printf("║                       Y88a  Y88o  Y88a   (     )               ║\n");
    printf("║                        Y88b  Y88b  Y88b   `.oo'                ║\n");
    printf("║                        :888  :888  :888  ( (`-'                ║\n");
    printf("║               .---.    d88P  d88P  d88P   `.`.                 ║\n");
    printf("║              / .-._)  d8P'\"\"\"|\"\"\"-Y8P       `.`.               ║\n");
    printf("║             ( (`._) .-.  .-. |.-.  .-.  .-.   ) )              ║\n");
    printf("║              \\ `---( O )( O )( O )( O )( O )-' /               ║\n");
    printf("║               `.    `-'  `-'  `-'  `-'  `-'  .'                ║\n");
    printf("║              ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~              ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("%s\n                        ENTER to return...", style[RESET]);
    while (_getch() != 13) {}
    clearInputBuffer();
}
void showPreparingAnimation() {
    // Rakibin hazırlanmasını beklerken kısa bir animasyon oynatır
    clearScreen();
    printf("%s                              %s\n", style[BG_LIGHTRED], style[RESET]);
    for (int i = 0; i < 10; i++) {
        printf("%s                              %s\n", style[BG_GRAY], style[RESET]);
    }
    gotoxy(31, 2);
    printf("       %s%s                                %s", style[RED], style[BG_GRAY], style[RESET]);
    gotoxy(31, 3);
    printf("       %s%s%s     Waiting for opponent       %s", style[WHITE], style[BOLD], style[BG_GRAY], style[RESET]);
    gotoxy(31, 4);
    printf("       %s%s                                %s", style[RED], style[BG_GRAY], style[RESET]);
    for (int i = 0; i < (rand() % 5) + 1; i++) {
        for (int j = 0; j < 3; j++) {
            gotoxy(63+j, 3);
            printf("%s.", style[BG_GRAY]);
            Sleep(350);
        }
        gotoxy(63, 3);
        printf("%s   ", style[BG_GRAY]);
    }
    
    printf("%s", style[RESET]);
    system("cls");
    clearScreen();
    clearInputBuffer();
}
void resetGameVariables() {
    // Oyun için global değişkenleri ilk değerlerine döndürür
    userShipCount = 0;
    opponentShipCount = 0;
    pointsUser = 0;
    pointsOpponent = 0;
    strategy.hitCount = 0;
    strategy.isVertical = -1;
    strategy.lastDirection = -1;
    
    for (int i = 0; i < 5; i++) {
        userShipStatus[i].destroyed = 0;
        opponentShipStatus[i].destroyed = 0;
    }
}