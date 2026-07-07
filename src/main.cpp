#include "ConsoleUI.h"
#include "RecordManager.h"

int main() {
    RecordManager manager("data/records.db");
    ConsoleUI ui(manager);
    ui.run();
    return 0;
}
