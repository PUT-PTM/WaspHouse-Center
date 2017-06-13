typedef unsigned char   BYTE;
typedef unsigned long	DWORD;
static volatile BYTE Timer1, Timer2;
unsigned int d, f;

/* Definitions for MMC/SDC command */
#define CMD0    (0x40+0)    /* GO_IDLE_STATE */
#define CMD1    (0x40+1)    /* SEND_OP_COND */
#define CMD8    (0x40+8)    /* SEND_IF_COND */
#define CMD9    (0x40+9)    /* SEND_CSD */
#define CMD10    (0x40+10)    /* SEND_CID */
#define CMD12    (0x40+12)    /* STOP_TRANSMISSION */
#define CMD16    (0x40+16)    /* SET_BLOCKLEN */
#define CMD17    (0x40+17)    /* READ_SINGLE_BLOCK */
#define CMD18    (0x40+18)    /* READ_MULTIPLE_BLOCK */
#define CMD23    (0x40+23)    /* SET_BLOCK_COUNT */
#define CMD24    (0x40+24)    /* WRITE_BLOCK */
#define CMD25    (0x40+25)    /* WRITE_MULTIPLE_BLOCK */
#define CMD41    (0x40+41)    /* SEND_OP_COND (ACMD) */
#define CMD55    (0x40+55)    /* APP_CMD */
#define CMD58    (0x40+58)    /* READ_OCR */

BYTE buffer[4096];	// Bufor przechowuj�cy kopiowane dane

// Funkcja znajduj�ca pliki w folderze przekazanym w parametrze
FRESULT scan_files (
    char* path        /* Start node to be scanned (also used as work area) */
)
{
	FIL fs, fd;
	UINT bw, br;
    FRESULT res, fr;
    FILINFO fno;
    DIR dir;
    char * path_new;	// �cie�ka dla podfolderu
    char * pathToFile;	// �cie�ka do pliku, kt�ry ma zosta� skopiowany
    char * pathToCopy;	// �cie�ka do miejsca gdzie ma zosta� skopiowany plik
    char * dirCopy = "Kopia";	// Folder do kt�rego kopiowane s� pliki
    char * private = "Private";
    char * newDir;	// �cie�ka folderu, kt�ry ma zosta� utworzony na drugim urz�dzeniu
    int i, j=0, n=0, m;
    char *fn;   /* This function is assuming non-Unicode cfg. */

#if _USE_LFN
    static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */

            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */

#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            //printf("fn: %s\n", fn);

            // Zapobiega wczytywaniu plik�w z folderu, do kt�rego pliki s� kopiowane
            if (strcmp(fn, dirCopy) == 0) continue;
            // Zapobiega wczytywaniu plik�w z folderu Private
            if (strcmp(fn, private) == 0) continue;
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
            	GPIO_SetBits(GPIOD, GPIO_Pin_14);

            	// Zapisanie scie�ki znalezionego folderu do zmiennej newDir
            	newDir = malloc(strlen(dirCopy)+strlen(path)+1+strlen(fn)+1);
            	strcpy(newDir, dirCopy);
            	strcat(newDir, path);
            	strcat(newDir, "/");
            	strcat(newDir, fn);

            	fr = f_mkdir(newDir);
            	free(newDir);

            	// Zapisanie �cie�ki podfolderu do zmiennej path_new
            	path_new = malloc(strlen(path)+strlen(fn)+1);
            	strcpy(path_new, path);
            	strcat(path_new, "/");
            	strcat(path_new, fn);

            	// Uruchomienie funkcji scan_files dla znalezionego folderu
                res = scan_files(path_new);
                free(path_new);
                if (res != FR_OK) break;
            } else {                                     /* It is a file. */
            	GPIO_SetBits(GPIOD, GPIO_Pin_13);
            	//printf("%s/%s\n", path, fn);

            	// Zapisanie �cie�ki znalezionego pliku do zmiennej pathToFile
            	pathToFile = malloc(strlen(path)+1+strlen(fn)+1);
            	strcpy(pathToFile, path);
            	strcat(pathToFile, "/");
            	strcat(pathToFile, fn);
            	//printf("pathToFile: %s\n", pathToFile);

            	// Zapisanie scie�ki gdzie ma zosta� skopiowany plik do zmiennej pathToCopy
            	pathToCopy = malloc(strlen(dirCopy)+strlen(path)+1+strlen(fn)+1);
            	strcpy(pathToCopy, dirCopy);
            	strcat(pathToCopy, path);
            	strcat(pathToCopy, "/");
            	strcat(pathToCopy, fn);
            	//printf("pathToCopy: %s\n", pathToCopy);

            	// Otworzenie znalezionego pliku
                fr = f_open(&fs, pathToFile, FA_OPEN_EXISTING | FA_READ);
                // Utworzenie pliku na drugim urzadzeniu gdzie maja zostac skopiowane dane
                fr = f_open(&fd, pathToCopy, FA_CREATE_ALWAYS | FA_WRITE);
                free(pathToCopy);
                free(pathToFile);

                /* Kopiowanie danych
                 * ------------------------------------------------------
                 * Kopiowany powinien by� tylko kawa�ek pliku. Plik err.file, pomimo tego, i�
                 * wa�y wi�cej ni� jest w stanie pomiesci� bufor, kopiowany jest poprawnie.
                 * Natomiast podczas kopiowania pliku .txt, kt�ry jest wi�kszy ni� bufor,
                 * program si� zawiesza.
                 *
                 * funkcja f_lseek()?
                 * ------------------------------------------------------
                 */
                for (;;) {
                    fr = f_read(&fs, buffer, sizeof buffer, &br);  // Read a chunk of source file
                    if (fr || br == 0) break; // error or eof
                    fr = f_write(&fd, buffer, br, &bw);            // Write it to the destination file
                    if (fr || bw < br) break; // error or disk full
                }

                // Zamkni�cie plik�w
                f_close(&fs);
                f_close(&fd);

            	f++;
            }
        }
        // Zamkni�cie folderu
        f_closedir(&dir);
    }

    return res;
}

static
BYTE rcvr_spi() 		// Odebranie bajtu z SD
{
  BYTE Data = 0;

	while( !( SPI2->SR & SPI_SR_TXE ));
	SPI2->DR = 0xFF;
	while( !( SPI2->SR & SPI_SR_RXNE ));
	Data = SPI2->DR;

  return Data;
}

static
BYTE wait_ready (void)
{
    BYTE res;

    Timer2 = 50;    /* Wait for ready in timeout of 500ms */
    rcvr_spi();
    do
        res = rcvr_spi();
    while ((res != 0xFF) && Timer2);

    return res;
}

static
void xmit_spi (BYTE Data)  // Wyslanie bajtu do SD
{
	BYTE nought;
	while( !( SPI2->SR & SPI_SR_TXE ));
	SPI2->DR = Data;
	while( !( SPI2->SR & SPI_SR_RXNE ));
	nought = SPI2->DR;

}

static
BYTE send_cmd (
    BYTE cmd,        /* Command byte */
    DWORD arg        /* Argument */
)
{
    BYTE n, res;

    if (wait_ready() != 0xFF) return 0xFF;

    /* Send command packet */
    xmit_spi(cmd);                        /* Command */
    xmit_spi((BYTE)(arg >> 24));        /* Argument[31..24] */
    xmit_spi((BYTE)(arg >> 16));        /* Argument[23..16] */
    xmit_spi((BYTE)(arg >> 8));            /* Argument[15..8] */
    xmit_spi((BYTE)arg);                /* Argument[7..0] */
    n = 0;
    if (cmd == CMD0) n = 0x95;            /* CRC for CMD0(0) */
    if (cmd == CMD8) n = 0x87;            /* CRC for CMD8(0x1AA) */
    xmit_spi(n);

    /* Receive command response */
    if (cmd == CMD12)  rcvr_spi();        /* Skip a stuff byte when stop reading */
    n = 10;                                /* Wait for a valid response in timeout of 10 attempts */
    do
        res = rcvr_spi();
    while ((res & 0x80) && --n);

    return res;            /* Return with the response value */
}
