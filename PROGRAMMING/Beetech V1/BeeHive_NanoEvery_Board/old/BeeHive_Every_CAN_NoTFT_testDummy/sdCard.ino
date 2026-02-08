#include <SPI.h>
#include <SD.h>

// set up variables using the SD utility library functions:
Sd2Card  card;
SdVolume volume;
SdFile   root;

// BeeHive MainBoard
const int chipSelect = 3;

File myFile;


/*
 * return 1  = init OK
 * return 0  = init Error
 * return -1 = config not found
 */
int sdCard_init(){

  Serial.print("Initializing SD card..");

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return 0;
  }
  Serial.println(".initialization success.");

  return 1;
 
}



int sdCard_readConfigFile_dummy(){

   stationNumber = STATIONUMBER;   
   strcpy( stationName , "LAB120" );
   sdcard_logging = 0;                
   sdcard_log_intervall = 60;    
   calibration_factor_int = 10000;    
   WEIGHT_OFFSET_float = 10.1;    

   return 1;
  
}

// safe memory
/*
void sdCard_CardInfo() {
  

  Serial.print("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    return ;
  } 
  
  Serial.println("Wiring is correct and a card is present.");

  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);

  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
  
}
*/
