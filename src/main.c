#include "app.h"

#include <stdio.h>

int main(void)
{
    int exit_code = 0;

    AppContext *app = app_create();
    if (app == NULL) {
        fprintf(stderr, "Failed to initialise application context.\n");
        return 1;
    }

    exit_code = app_run(app);
    if (exit_code != 0) {
        fprintf(stderr, "Application exited with status %d.\n", exit_code);
    }

    app_destroy(app);
    return exit_code;
}
