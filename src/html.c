#include <stdio.h>
#include <stdlib.h>

char *HTTP_NotFound() {
    char *ret = malloc(1024);
    FILE *fp = fopen("./html/404.html", "r");
    fread(ret, 1, 1024, fp);
    fclose(fp);
    return ret;
}

char *HTTP_Forbidden() {
    char *ret = malloc(1024);
    FILE *fp = fopen("./html/403.html", "r");
    fread(ret, 1, 1024, fp);
    fclose(fp);
    return ret;
}
