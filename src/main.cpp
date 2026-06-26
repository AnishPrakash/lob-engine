#include "matching_engine.hpp"
#include "replay_engine.hpp"
#include "fix_output.hpp"
#include <ncurses.h>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <fmt/format.h>

static std::atomic<bool> g_quit(false);

// Function to render the LOB state to the ncurses window
void draw_book(const MatchingEngine& eng, const ReplayEngine& replay) {
    clear();
    int row = 1;
    
    attron(A_BOLD | COLOR_PAIR(1));
    mvprintw(0, 0, "LOB ENGINE | Msg: %lu | Active Orders: %u | Press 'q' to quit", replay.messages_processed(), eng.active_orders());
    attroff(A_BOLD | COLOR_PAIR(1));
    
    mvprintw(row++, 2, "%10s %10s", "PRICE", "QTY");
    
    // --- ASKS ---
    attron(COLOR_PAIR(3)); // Red for Asks
    mvprintw(row++, 2, "--- ASKS ---");
    const HalfBook& asks = eng.asks();
    uint32_t ask_pi = asks.best_ask_idx();
    int shown = 0;
    
    if (ask_pi == NULL_IDX) mvprintw(row++, 4, "(Empty)");
    
    while (ask_pi != NULL_IDX && shown < 10) {
        // Dividing by 100 to convert cents back to displayable dollar format
        double px = ask_pi / 100.0; 
        mvprintw(row++, 2, "%10.2f %10lu", px, asks.levels[ask_pi].total_qty);
        
        uint32_t next_pi = NULL_IDX;
        for (uint32_t p = ask_pi + 1; p < MAX_PRICE_LEVELS; ++p) {
            if (asks.is_active(p)) { next_pi = p; break; }
        }
        ask_pi = next_pi;
        ++shown;
    }
    attroff(COLOR_PAIR(3));
    
    // --- BIDS ---
    attron(COLOR_PAIR(2)); // Green for Bids
    mvprintw(row++, 2, "--- BIDS ---");
    const HalfBook& bids = eng.bids();
    uint32_t bid_pi = bids.best_bid_idx();
    shown = 0;
    
    if (bid_pi == NULL_IDX) mvprintw(row++, 4, "(Empty)");

    while (bid_pi != NULL_IDX && shown < 10) {
        double px = bid_pi / 100.0;
        mvprintw(row++, 2, "%10.2f %10lu", px, bids.levels[bid_pi].total_qty);
        
        uint32_t next_pi = NULL_IDX;
        for (int p = (int)bid_pi - 1; p >= 0; --p) {
            if (bids.is_active(p)) { next_pi = p; break; }
        }
        bid_pi = next_pi;
        ++shown;
    }
    attroff(COLOR_PAIR(2));
    refresh();
}

int main(int argc, char* argv[]) {
    if (argc < 2) { 
        fmt::print("Usage: lob_engine <itch_file>\n"); 
        return 1; 
    }
    
    MatchingEngine engine([](const Fill& f) {});
    
    // Initialize replay engine
    ReplayEngine replay(engine, 10.0); 
    std::thread replay_thread([&]() { replay.start(argv[1]); });
    
    // Ncurses Setup
    initscr(); 
    cbreak(); 
    noecho(); 
    curs_set(0);
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_CYAN); 
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK); 
    nodelay(stdscr, TRUE);
    
    while (!g_quit) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') { 
            g_quit = true; 
            replay.stop(); 
        }
        // Update the display with current engine and replay state
        draw_book(engine, replay);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    endwin();
    replay_thread.join();
    fmt::print("Processed {} messages\n", replay.messages_processed());
    return 0;
}