/* Standalone harness tests link control.c without main.c/spools.h. */
int spools_tray_insert_stabilizing(int slot) {
    (void)slot;
    return 0;
}
