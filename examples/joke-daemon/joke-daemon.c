#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

static volatile int keep_running = 1;

void signal_handler(int signum) {
    (void)signum;  // Suppress unused parameter warning
    keep_running = 0;
}

const char* jokes[] = {
    "Why do programmers prefer dark mode? Because light attracts bugs!",
    "Why did the developer go broke? Because he used up all his cache!",
    "How many programmers does it take to change a light bulb? None, that's a hardware problem!",
    "Why do Java developers wear glasses? Because they don't C#!",
    "What's a programmer's favorite hangout place? The Foo Bar!",
    "Why did the programmer quit his job? He didn't get arrays!",
    "What do you call a programmer from Finland? Nils!",
    "Why do programmers always mix up Halloween and Christmas? Because Oct 31 == Dec 25!",
    "What's the object-oriented way to become wealthy? Inheritance!",
    "Why did the function break up with the variable? Because it had too many arguments!",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Hi from Talos",
    "Segmentation fault. core dump:"
};

int main() {
    const int num_jokes = sizeof(jokes) / sizeof(jokes[0]);

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    srand(time(NULL));

    printf("Joke Daemon started - printing random jokes every 60 seconds\n");

    while (keep_running) {
        int random_index = rand() % num_jokes;
        printf("JOKE: %s\n", jokes[random_index]);
        fflush(stdout);

        // Sleep for 60 seconds, but check for signals every second
        for (int i = 0; i < 60 && keep_running; i++) {
            sleep(1);
        }
    }

    printf("Joke Daemon shutting down gracefully\n");
    return 0;
}
