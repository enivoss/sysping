#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <conio.h>
#include <cctype>
#include <numeric>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

struct Host {
    std::string label;
    std::string address;
    std::vector<DWORD> history;
    bool reachable;
    DWORD last;
};

void setColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void clearScreen() {
    COORD topLeft = {0, 0};
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;
    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    FillConsoleOutputAttribute(console, screen.wAttributes, screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    SetConsoleCursorPosition(console, topLeft);
}

DWORD pingHost(const std::string& address) {
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) return 0xFFFFFFFF;

    unsigned long ipAddr = inet_addr(address.c_str());
    if (ipAddr == INADDR_NONE) {
        struct hostent* he = gethostbyname(address.c_str());
        if (!he) { IcmpCloseHandle(hIcmp); return 0xFFFFFFFF; }
        ipAddr = *(unsigned long*)he->h_addr;
    }

    char sendData[32] = "sysping";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    void* replyBuf = malloc(replySize);

    DWORD result = IcmpSendEcho(hIcmp, ipAddr, sendData, sizeof(sendData),
                                 nullptr, replyBuf, replySize, 1000);

    DWORD rtt = 0xFFFFFFFF;
    if (result > 0) {
        ICMP_ECHO_REPLY* reply = (ICMP_ECHO_REPLY*)replyBuf;
        if (reply->Status == IP_SUCCESS) rtt = reply->RoundTripTime;
    }

    free(replyBuf);
    IcmpCloseHandle(hIcmp);
    return rtt;
}

std::string sparkBar(const std::vector<DWORD>& hist) {
    if (hist.empty()) return "";
    const char* blocks[] = {" ", "_", ".", ":", "|", "I", "H"};
    DWORD maxVal = 0;
    for (auto v : hist) if (v != 0xFFFFFFFF && v > maxVal) maxVal = v;
    if (maxVal == 0) maxVal = 1;

    std::string bar;
    int show = (int)hist.size() > 20 ? 20 : (int)hist.size();
    int start = (int)hist.size() - show;
    for (int i = start; i < (int)hist.size(); i++) {
        if (hist[i] == 0xFFFFFFFF) { bar += "x"; continue; }
        int idx = (int)(((double)hist[i] / maxVal) * 6);
        if (idx > 6) idx = 6;
        bar += blocks[idx];
    }
    while ((int)bar.size() < 20) bar = " " + bar;
    return bar;
}

double avgRtt(const std::vector<DWORD>& hist) {
    if (hist.empty()) return 0;
    double sum = 0; int count = 0;
    for (auto v : hist) if (v != 0xFFFFFFFF) { sum += v; count++; }
    return count > 0 ? sum / count : 0;
}

void drawUI(const std::vector<Host>& hosts, int selected, int tick) {
    setColor(0x0A);
    std::cout << "  sysping";
    setColor(0x08);
    std::cout << " by enivoss";
    setColor(0x0A);
    std::cout << "   tick " << tick << "\n";
    setColor(0x08);
    std::cout << "  ------------------------------------------------------------------------\n";
    setColor(0x07);
    std::cout << "  " << std::left
              << std::setw(16) << "HOST"
              << std::setw(18) << "ADDRESS"
              << std::setw(10) << "LAST"
              << std::setw(10) << "AVG"
              << std::setw(22) << "HISTORY"
              << "\n";
    setColor(0x08);
    std::cout << "  ------------------------------------------------------------------------\n";

    for (int i = 0; i < (int)hosts.size(); i++) {
        const Host& h = hosts[i];
        bool sel = (i == selected);

        if (sel) setColor(0x20);
        else if (!h.reachable) setColor(0x0C);
        else if (h.last > 100) setColor(0x0E);
        else setColor(0x07);

        std::string lastStr = h.reachable ? std::to_string(h.last) + "ms" : "timeout";
        std::string avgStr  = h.history.empty() ? "-" :
                              std::to_string((int)avgRtt(h.history)) + "ms";

        std::cout << "  " << std::left
                  << std::setw(16) << h.label
                  << std::setw(18) << h.address
                  << std::setw(10) << lastStr
                  << std::setw(10) << avgStr
                  << sparkBar(h.history)
                  << "\n";
    }

    setColor(0x08);
    std::cout << "\n  ------------------------------------------------------------------------\n";
    setColor(0x07);
    std::cout << "  [UP/DOWN] navigate   [Q] quit\n";
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    SetConsoleTitleA("sysping");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hConsole, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &ci);

    std::vector<Host> hosts = {
        {"cloudflare",  "1.1.1.1",       {}, false, 0},
        {"google dns",  "8.8.8.8",       {}, false, 0},
        {"google",      "google.com",    {}, false, 0},
        {"github",      "github.com",    {}, false, 0},
        {"opendns",     "208.67.222.222",{}, false, 0},
    };

    int selected = 0;
    int tick = 0;

    while (true) {
        for (auto& h : hosts) {
            DWORD rtt = pingHost(h.address);
            h.last = rtt;
            h.reachable = (rtt != 0xFFFFFFFF);
            h.history.push_back(rtt);
            if (h.history.size() > 60) h.history.erase(h.history.begin());
        }
        tick++;

        clearScreen();
        drawUI(hosts, selected, tick);

        DWORD startWait = GetTickCount();
        while (GetTickCount() - startWait < 1000) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 0 || key == 224) {
                    key = _getch();
                    if (key == 72 && selected > 0) selected--;
                    else if (key == 80 && selected < (int)hosts.size() - 1) selected++;
                } else {
                    if ((char)tolower(key) == 'q') goto done;
                }
            }
            Sleep(50);
        }
    }

done:
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &ci);
    setColor(0x07);
    clearScreen();
    WSACleanup();
    return 0;
}
