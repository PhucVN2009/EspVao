const char *GetNameActors(int id) {
    static char nameBuf[16];
    snprintf(nameBuf, sizeof(nameBuf), "%d", id);
    switch (id){
        case 105:
            return "Toro";
            break;
        case 106:
            return "Krixi";
            break;
        case 107:
            return "Zephys";
            break;
        case 108:
            return "Gildur";
            break;
        case 109:
            return "Veera";
            break;
        case 110:
            return "Kahi";
            break;
        case 111:
            return "Violet";
            break;
        case 112:
            return "Yorn";
            break;
        case 113:
            return "Chaugnar";
            break;
        case 114:
            return "Omega";
            break;
        case 115:
            return "Jinna";
            break;
        case 116:
            return "Butterfly";
            break;
        case 117:
            return "Ormarr";
            break;
        case 118:
            return "Alice";
            break;
        case 119:
            return "Mganga";
            break;
        case 120:
            return "Mina";
            break;
        case 121:
            return "Marja";
            break;
        case 123:
            return "Maloch";
            break;
        case 124:
            return "Ignis";
            break;
        case 126:
            return "Arduin";
            break;
        case 127:
            return "Azzen'Ka";
            break;
        case 128:
            return "Lữ Bố";
            break;
        case 129:
            return "Triệu Vân";
            break;
        case 130:
            return "Airi";
            break;
        case 131:
            return "Murad";
            break;
        case 132:
            return "Hayate";
            break;
        case 133:
            return "Valhein";
            break;
        case 134:
            return "Skud";
            break;
        case 135:
            return "Thane";
            break;
        case 136:
            return "Ilumia";
            break;
        case 137:
            return "Paine";
            break;
        case 139:
            return "Kil'Groth";
            break;
        case 140:
            return "SuperMan";
            break;
        case 141:
            return "Lauriel";
            break;
        case 142:
            return "Natalya";
            break;
        case 144:
            return "Taara";
            break;
        case 146:
            return "Zill";
            break;
        case 148:
            return "Preyta";
            break;
        case 149:
            return "Xeniel";
            break;
        case 150:
            return "Nakroth";
            break;
        case 152:
            return "Điêu Thuyền";
            break;
        case 153:
            return "Batman";
            break;
        case 154:
            return "Yena";
            break;
        case 156:
            return "Aleister";
            break;
        case 157:
            return "Raz";
            break;
        case 162:
            return "Kriknak";
            break;
        case 163:
            return "Ryoma";
            break;
        case 166:
            return "Arthur";
            break;
        case 167:
            return "Ngộ Không";
            break;
        case 168:
            return "Lumburr";
            break;
        case 169:
            return "Slimz";
            break;
        case 170:
            return "Moren";
            break;
        case 171:
            return "Cresht";
            break;
        case 173:
            return "Fennik";
            break;
        case 174:
            return "Stuart";
            break;
        case 175:
            return "Grakk";
            break;
        case 177:
            return "Lindis";
            break;
        case 180:
            return "Max";
            break;
        case 184:
            return "Helen";
            break;
        case 186:
            return "TeeMee";
            break;
        case 187:
            return "Arum";
            break;
        case 189:
            return "Krizzix";
            break;
        case 190:
            return "Tulen";
            break;
        case 191:
            return "Rouie";
            break;
        case 192:
            return "Celica";
            break;
        case 193:
            return "Amily";
            break;
        case 194:
            return "Wiro";
            break;
        case 195:
            return "Enzo";
            break;
        case 196:
            return "Elsu";
            break;
        case 199:
            return "Eland'orr";
            break;
        case 501:
            return "Tel'Annas";
            break;
        case 502:
            return "Asrid";
            break;
        case 503:
            return "Zuka";
            break;
        case 504:
            return "Wonder Woman";
            break;
        case 505:
            return "Baldum";
            break;
        case 506:
            return "Omen";
            break;
        case 507:
            return "Flash";
            break;
        case 508:
            return "Wisp";
            break;
        case 509:
            return "Y'bneth";
            break;
        case 510:
            return "Liliana";
            break;
        case 511:
            return "Ata";
            break;
        case 512:
            return "Rourke";
            break;
        case 513:
            return "Zata";
            break;
        case 515:
            return "Richter";
            break;
        case 519:
            return "Annette";
            break;
        case 518:
            return "Quillen";
            break;
        case 520:
            return "Veres";
            break;
        case 521:
            return "Florentino";
            break;
        case 522:
            return "Errol";
            break;
        case 523:
            return "D'arcy";
            break;
        case 524:
            return "Capheny";
            break;
        case 525:
            return "Zip";
            break;
        case 526:
            return "Ishar";
            break;
        case 527:
            return "Sephera";
            break;
        case 528:
            return "QI";
            break;
        case 529:
            return "Volkath";
            break;
        case 530:
            return "Dirak";
            break;
        case 531:
            return "Keera";
            break;
        case 532:
            return "Thorne";
            break;
        case 533:
            return "Laville";
            break;
        case 534:
            return "Dextra";
            break;
        case 535:
            return "Sinestrea";
            break;
        case 536:
            return "Aoi";
            break;
        case 537:
            return "Allain";
            break;
        case 538:
            return "Iggy";
            break;
        case 539:
            return "Laurion";
            break;
        case 540:
            return "Bright";
            break;
        case 541:
            return "Bonie";
            break;
        case 542:
            return "Tachi";
            break;
        case 543:
            return "Aya";
            break;
        case 544:
            return "Yan";
            break;
        case 545:
            return "Yue";
            break;
        case 546:
            return "Terri";
            break;
        case 548:
            return "Bijan";
            break;
        case 568:
            return "Ming";
            break;
        case 159:
            return "Dolia";
            break;
        case 206:
            return "Charlotte";
            break;
        case 563:
            return "Heino";
            break;
        case 567:
            return "Erin";
            break;
        case 577:
            return "Dyadia";
            break;
        case 582:
            return "Flowborn2";
            break;
        case 584:
            return "Flowborn";
            break;
        case 595:
            return "Edras";
            break;
        case 596:
            return "Goverra";
            break;
        case 597:
            return "Biron";
            break;
        case 598:
            return "Bolt Baron";
            break;
        case 599:
            return "Billow";
            break;
        case 808:
            return "BOT - Athur";
            break;
        case 809:
            return "BOT - Tel'Annas";
            break;
        default:
            return nameBuf;
    }
}