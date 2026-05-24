#pragma once

ProcMap anogsMap;
void *AnogsMemcpyThread(void *) {
    while (!anogsMap.isValid()) {
	    anogsMap = KittyMemory::getLibraryBaseMap("libanogs.so");
        sleep(1);
    }
    MemoryPatch::createWithHex(anogsMap, 0x71568, "D5 E7 00 20").Modify();
    MemoryPatch::createWithHex(anogsMap, 0x7156C, "70 47 2D E9").Modify();
    MemoryPatch::createWithHex(anogsMap, 0x73544, "1E FF 2F E1").Modify();
    MemoryPatch::createWithHex(anogsMap, 0x851D0, "20 46 00 BF").Modify();
    MemoryPatch::createWithHex(anogsMap, 0x851D4, "00 BF 20 46").Modify();
    return NULL;
}
