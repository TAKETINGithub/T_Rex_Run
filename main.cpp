#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>

// プラットフォーム固有の機能のためのヘッダ
#ifdef _WIN32
#include <conio.h> // _kbhit(), _getch()
#include <windows.h> // Sleep(), SetConsoleCursorPosition()
void clearScreen() {
    system("cls");
}
void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
#else
#include <unistd.h> // usleep()
#include <termios.h> // termios, tcsetattr, tcgetattr
// ncursesを使わずにkbhitとgetchを実装
int _kbhit() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}
int _getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
void clearScreen() {
    // ANSIエスケープシーケンスを使って画面をクリアし、カーソルを左上に移動
    std::cout << "\033[2J\033[1;1H";
}
void setCursorPosition(int x, int y) {
    // ANSIエスケープシーケンスを使ってカーソルを移動
    printf("\033[%d;%dH", y + 1, x + 1);
}
void Sleep(int milliseconds) {
    usleep(milliseconds * 1000);
}
#endif

// --- ゲーム設定 ---
const int SCREEN_WIDTH = 80;
const int SCREEN_HEIGHT = 20;
const int GROUND_Y = SCREEN_HEIGHT - 2;

// --- プレイヤークラス ---
class Player {
public:
    int x, y;
    float dy; // y方向の速度
    bool isJumping;
    std::string sprite[3] = {
        "  O",
        " /\\/",
        " L L"
    };

    Player() : x(5), y(GROUND_Y - 2), dy(0), isJumping(false) {}

    void jump() {
        if (!isJumping) {
            isJumping = true;
            dy = -2.0f; // ジャンプの初速
        }
    }

    void update() {
        if (isJumping) {
            y += dy;
            dy += 0.40f; // 重力

            if (y >= GROUND_Y - 2) {
                y = GROUND_Y - 2;
                isJumping = false;
            }
        }
    }

    void draw(std::vector<std::string>& screen) {
        for (int i = 0; i < 3; ++i) {
            if (y + i >= 0 && y + i < SCREEN_HEIGHT) {
                for(int j = 0; j < sprite[i].length(); ++j) {
                    if (x + j >= 0 && x + j < SCREEN_WIDTH) {
                        screen[y + i][x + j] = sprite[i][j];
                    }
                }
            }
        }
    }
};

// --- 障害物クラス ---
class Obstacle {
public:
    int x, y;
    std::string sprite;

    Obstacle(int startX) : x(startX) {
        // ランダムでサボテンの種類を決める
        if (rand() % 2 == 0) {
            sprite = "T";
            y = GROUND_Y - 1;
        } else {
            sprite = "Y";
            y = GROUND_Y - 1;
        }
    }

    void update() {
        x--;
    }

    void draw(std::vector<std::string>& screen) {
        if (y >= 0 && y < SCREEN_HEIGHT && x >= 0 && x < SCREEN_WIDTH) {
            screen[y][x] = sprite[0];
        }
    }
};

// --- メイン関数 ---
int main() {
    srand(time(0)); // 乱数のシードを設定

    bool gameStarted = false;
    while (!gameStarted) {
        // --- 描画処理（ダブルバッファリング） ---
        std::vector<std::string> screen(SCREEN_HEIGHT, std::string(SCREEN_WIDTH, ' '));

        // T-RexのASCIIアート
        std::string trex_line1 = "        __";
        std::string trex_line2 = "       /..\\";
        std::string trex_line3 = "      (_.__)";
        std::string trex_line4 = "      /| |";
        std::string trex_line5 = "     //_//";
        std::string trex_line6 = "    \\___/";

        screen[SCREEN_HEIGHT / 2 - 6].replace(SCREEN_WIDTH / 2 - 10, trex_line1.length(), trex_line1);
        screen[SCREEN_HEIGHT / 2 - 5].replace(SCREEN_WIDTH / 2 - 10, trex_line2.length(), trex_line2);
        screen[SCREEN_HEIGHT / 2 - 4].replace(SCREEN_WIDTH / 2 - 10, trex_line3.length(), trex_line3);
        screen[SCREEN_HEIGHT / 2 - 3].replace(SCREEN_WIDTH / 2 - 10, trex_line4.length(), trex_line4);
        screen[SCREEN_HEIGHT / 2 - 2].replace(SCREEN_WIDTH / 2 - 10, trex_line5.length(), trex_line5);
        screen[SCREEN_HEIGHT / 2 - 1].replace(SCREEN_WIDTH / 2 - 10, trex_line6.length(), trex_line6);


        // タイトル
        std::string titleText = "T - R E X   R U N";
        screen[SCREEN_HEIGHT / 2 + 2].replace(SCREEN_WIDTH / 2 - titleText.length() / 2, titleText.length(), titleText);

        // 操作案内
        std::string guideText = "Press SPACE to Start";
        screen[SCREEN_HEIGHT / 2 + 4].replace(SCREEN_WIDTH / 2 - guideText.length() / 2, guideText.length(), guideText);

        // バックバッファの内容をまとめて画面に出力
        setCursorPosition(0, 0);
        std::string buffer;
        buffer.reserve(SCREEN_WIDTH * SCREEN_HEIGHT + 100);
        for (const auto& line : screen) {
            buffer.append(line).append("\n");
        }
        std::cout << buffer << std::flush;

        // --- 入力処理 ---
        if (_kbhit()) {
            char key = _getch();
            if (key == ' ') { // スペースキーが押されたら
                gameStarted = true; // スタート画面ループを抜ける
            }
            if (key == 'q' || key == 'Q') { // Qキーでそのまま終了
                return 0;
            }
        }
        Sleep(100);
    }

    Player player;
    std::vector<Obstacle> obstacles;
    int score = 0;
    int frameCount = 0;
    bool gameOver = false;

    // ゲームループ
    while (!gameOver) {
        // --- 入力処理 ---
        if (_kbhit()) {
            char key = _getch();
            if (key == ' ' || key == 'w' || key == 'W') {
                player.jump();
            }
            if (key == 'q' || key == 'Q') { // 'q'で終了
                break;
            }
        }

        // --- ゲームロジックの更新 ---
        player.update();
        score++;

        // 障害物の生成
        if (frameCount % (40 + rand() % 40) == 0) {
            obstacles.push_back(Obstacle(SCREEN_WIDTH - 2));
        }

        // 障害物の更新と削除
        for (int i = 0; i < obstacles.size(); ++i) {
            obstacles[i].update();
            // 画面外に出た障害物を削除
            if (obstacles[i].x < 0) {
                obstacles.erase(obstacles.begin() + i);
                i--;
            }
        }

        // --- 当たり判定 ---
        for (const auto& obs : obstacles) {
            // 簡単な矩形での当たり判定
            if (player.x + 3 > obs.x && player.x < obs.x + 1 &&
                player.y + 2 > obs.y && player.y < obs.y + 1)
            {
                gameOver = true;
            }
        }

        // --- 描画処理 ---
        std::vector<std::string> screen(SCREEN_HEIGHT, std::string(SCREEN_WIDTH, ' '));

        // 地面を描画
        for (int i = 0; i < SCREEN_WIDTH; ++i) {
            screen[GROUND_Y][i] = '-';
        }

        // プレイヤーを描画
        player.draw(screen);

        // 障害物を描画
        for (auto& obs : obstacles) {
            obs.draw(screen);
        }

        // スコアを描画
        std::string scoreText = "Score: " + std::to_string(score);
        screen[0].replace(2, scoreText.length(), scoreText);

        // 画面をクリアせず、カーソルを左上に移動させる
        setCursorPosition(0, 0);

        // 描画バッファの内容を一つの文字列にまとめてから、一気に出力する
        std::string buffer;
        // 事前に十分なメモリを確保しておくと、処理が少し速くなります
        buffer.reserve(SCREEN_WIDTH * SCREEN_HEIGHT + 100);
        for (int i = 0; i < SCREEN_HEIGHT; ++i) {
            buffer.append(screen[i]);
            buffer.append("\n");
        }
        buffer.append("Press SPACE to Jump, Q to Quit\n");
        std::cout << buffer << std::flush; // std::flushでバッファを即座に出力


        // --- フレームレート制御 ---
        Sleep(30); // 約33FPS
        frameCount++;
    }
    clearScreen();

    bool showResult = true;
    while (showResult) {
        // --- 描画処理（ダブルバッファリング） ---
        // 1. バックバッファを作成
        std::vector<std::string> screen(SCREEN_HEIGHT, std::string(SCREEN_WIDTH, ' '));

        // 2. 描画内容をすべてバックバッファに書き込む
        // ASCIIアートの各行を定義
        std::string line1 = " ____                         ___";
        std::string line2 = "/ ___| __ _ _ __ ___   ___   / _ \\__   _____ _ __";
        std::string line3 = "| |  _ / _` | '_ ` _ \\ / _ \\ | | | \\ \\ / / _ \\ '__|";
        std::string line4 = "| |_| | (_| | | | | | |  __/ | |_| |\\ V /  __/ |";
        std::string line5 = " \\____|\\__,_|_| |_| |_|\\___|  \\___/  \\_/ \\___|_|";

        // バッファの指定した位置に文字列を書き込む
        screen[SCREEN_HEIGHT / 2 - 4].replace(SCREEN_WIDTH / 2 - 25, line1.length(), line1);
        screen[SCREEN_HEIGHT / 2 - 3].replace(SCREEN_WIDTH / 2 - 25, line2.length(), line2);
        screen[SCREEN_HEIGHT / 2 - 2].replace(SCREEN_WIDTH / 2 - 25, line3.length(), line3);
        screen[SCREEN_HEIGHT / 2 - 1].replace(SCREEN_WIDTH / 2 - 25, line4.length(), line4);
        screen[SCREEN_HEIGHT / 2 - 0].replace(SCREEN_WIDTH / 2 - 25, line5.length(), line5);

        // スコア表示
        std::string scoreText = "Your Score: " + std::to_string(score);
        screen[SCREEN_HEIGHT / 2 + 3].replace(SCREEN_WIDTH / 2 - scoreText.length() / 2, scoreText.length(), scoreText);

        // 操作案内
        std::string guideText = "Press 'R' to Retry, 'Q' to Quit";
        screen[SCREEN_HEIGHT / 2 + 5].replace(SCREEN_WIDTH / 2 - guideText.length() / 2, guideText.length(), guideText);

        // 3. バックバッファの内容をまとめて画面に出力
        setCursorPosition(0, 0);
        std::string buffer;
        buffer.reserve(SCREEN_WIDTH * SCREEN_HEIGHT + 100);
        for (const auto& line : screen) {
            buffer.append(line).append("\n");
        }
        std::cout << buffer << std::flush;


        // --- 入力処理 ---
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q' || key == 'Q') {
                showResult = false; // ループを抜けて終了
            }
            if (key == 'r' || key == 'R') {
                main(); // ゲームを再スタート
                return 0; // 再帰呼び出しから戻ってきたら終了
            }
        }
        Sleep(100); // CPU負荷を少し下げる
    }
    setCursorPosition(0, SCREEN_HEIGHT); // カーソルを画面下部に移動
    return 0;
}
