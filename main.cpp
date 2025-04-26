#include <thread>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <mutex>
#include <condition_variable>
#include <numeric>

using namespace std;

const int ROUNDS = 10;

vector<int> cards_template = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                              2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
mutex mtx;
condition_variable cv;
bool shuffle_done = false;
bool player1_done = false;
bool player2_done = false;
bool write_done = true;

vector<int> cards;
vector<int> dealer_cards, player1_cards, player2_cards;

void shuffle_cards() {
    random_device rd;
    mt19937 g(rd());
    for (int round = 1; round <= ROUNDS; ++round) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return write_done; });
        cards = cards_template;
        shuffle(cards.begin(), cards.end(), g);
        dealer_cards = cards;
        shuffle_done = true;
        write_done = false;
        cv.notify_all();
    }
}

void player_pick(vector<int>& player_cards, bool& current_done, bool& previous_done, int player_number) {
    for (int round = 1; round <= ROUNDS; ++round) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&] { return previous_done; });
        if (cards.size() >= 5) {
            player_cards.assign(cards.begin(), cards.begin() + 5);
            cards.erase(cards.begin(), cards.begin() + 5);
        }
        current_done = true;
        previous_done = false;
        cv.notify_all();
    }
}


void write_results() {
    ofstream outfile("results.txt");
    for (int round = 1; round <= ROUNDS; ++round) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return player2_done; });
        int player1_sum = accumulate(player1_cards.begin(), player1_cards.end(), 0);
        int player2_sum = accumulate(player2_cards.begin(), player2_cards.end(), 0);
        outfile << "Round " << round << ":\n";
        outfile << "Dealer shuffled cards: ";
        for (int card : dealer_cards) {
            outfile << card << " ";
        }
        outfile << "\nPlayer 1 cards: ";
        for (int card : player1_cards) {
            outfile << card << " ";
        }
        outfile << "\nPlayer 2 cards: ";
        for (int card : player2_cards) {
            outfile << card << " ";
        }
        if (player1_sum > player2_sum) {
            outfile << "\nPlayer 1 wins!\n\n";
        }
        else if (player1_sum < player2_sum) {
            outfile << "\nPlayer 2 wins!\n\n";
        }
        else {
            outfile << "\nIt's a draw!\n\n";
        }
        player2_done = false;
        write_done = true;
        cv.notify_all();
    }

    outfile.close();
}

int main() {
    thread t1(shuffle_cards);
    thread t2(player_pick, ref(player1_cards), ref(player1_done), ref(shuffle_done), 1);
    thread t3(player_pick, ref(player2_cards), ref(player2_done), ref(player1_done), 2);
    thread t4(write_results);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return 0;
}
