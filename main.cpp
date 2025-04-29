#include <thread>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <mutex>
#include <condition_variable>
#include <numeric>

using namespace std;

const int ROUNDS = 1000;

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

        player_cards.push_back(dealer_cards[0]);
        player_cards.push_back(dealer_cards[1]);
        dealer_cards.erase(dealer_cards.begin(), dealer_cards.begin() + 2);

        int it = 0;
        while (true) {
            int sum = accumulate(player_cards.begin(), player_cards.end(), 0);
            if (sum == 21 || sum == 20 || sum > 21) break;
            else if (sum <= 15) {
                player_cards.push_back(dealer_cards[it]);
                dealer_cards.erase(dealer_cards.begin() + it);
                it++;
                continue;
            }
            else if (sum <= 17 && rand() % 8 == 2) {
                player_cards.push_back(dealer_cards[it]);
                dealer_cards.erase(dealer_cards.begin() + it);
                break;
            }
            else if (sum <= 19 && rand() % 15 == 4) {
                player_cards.push_back(dealer_cards[it]);
                dealer_cards.erase(dealer_cards.begin() + it);
                break;
            }
        }
        current_done = true;
        previous_done = false;
        cv.notify_all();
    }
}

void write_results() {
    ofstream outfile("results.txt");
    int player1Rounds = 0, player2Rounds = 0, draws = 0;
    for (int round = 1; round <= ROUNDS; ++round) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return player2_done; });
        int player1_sum = accumulate(player1_cards.begin(), player1_cards.end(), 0);
        int player2_sum = accumulate(player2_cards.begin(), player2_cards.end(), 0);
        outfile << "Round " << round << ":\n";
        outfile << "Dealer shuffled cards: ";
        for (int card : cards) {
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
        if ((player1_sum > player2_sum || player2_sum > 21) && player1_sum <= 21) {
            outfile << "\nPlayer 1 wins!\n\n";
            player1Rounds++;
        }
        else if ((player1_sum < player2_sum || player1_sum > 21) && player2_sum <= 21) {
            outfile << "\nPlayer 2 wins!\n\n";
            player2Rounds++;
        }
        else {
            outfile << "\nIt's a draw!\n\n";
            draws++;
        }

        player1_cards.clear();
        player2_cards.clear();

        player2_done = false;
        write_done = true;
        cv.notify_all();
    }
    outfile << "Player 1 win Rounds: " << player1Rounds << "\n";
    outfile << "Player 2 win Rounds: " << player2Rounds << "\n";
    outfile << "Calculating...\n";
    if (player1Rounds > player2Rounds) outfile << "Player 1 Win!\n";
    else if (player1Rounds < player2Rounds) outfile << "Player 2 Win!\n";
    else outfile << "Draw!\n";
    outfile << "Number of draws: " << draws;

    outfile.close();
}

int main() {
    srand(time(0));

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