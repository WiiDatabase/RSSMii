#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <mxml.h>

#include "wc24/wc24.h"

#define MAX_BUF 511

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

u64 rssmiititleid = 0x0001000846454544; // Title ID for RSS-Feed mail: 00010008-FEED.
nwc24dl_record nwc24record;
nwc24dl_entry nwc24entry;
struct RSS_Job {
    char url[256];
    char name[256];
    char final_url[MAX_BUF + 1];
} rssjob;

RSS_Job * jobs;
int ijobs = 0;

static char _buffer[MAX_BUF + 1];
static const char *_hex = "0123456789abcdef";
//char * server_url;


char *url_encode(char *str, char *buf, int max) {
    int c;
    char *result = buf;
    while (max > 0) {
        c = *str++;
        if (!c)
            break;
        if (('a' <= c && c <= 'z')
            || ('A' <= c && c <= 'Z')
            || ('0' <= c && c <= '9')) {
            *buf++ = c;
            max--;
        } else {
            *buf++ ='%';
            *buf++ = _hex[c >> 4];
            *buf++ = _hex[c & 15];
            max -= 3;
        }
    }
    *buf = '\0';
    return result;
}

int load_feeds() {
    mxml_node_t *tree;
    mxml_node_t *rss;
    mxml_node_t *node;

    // Initialize libfat
    if (!fatInitDefault()) {
        return -1;
    }
    
    // Open feeds.xml file
    FILE *fp = fopen("/feeds.xml", "rb");
    if (fp == NULL) {
        fclose(fp);
        return -2;
    }
    fseek(fp, 0, SEEK_END);
    long settings_size = ftell (fp);
    rewind(fp);
    if (settings_size < 0) {
        fclose(fp);
        return -3;
    }
    
    // Load XML into structure
    tree = mxmlLoadFile(NULL, fp, MXML_OPAQUE_CALLBACK);
    fclose(fp);
    
    rss = mxmlFindElement(tree, tree, "rss", NULL, NULL, MXML_DESCEND);
    if (rss == NULL) {
        return -103;
    }
    
    // TODO
    /*server_url = mxmlElementGetAttr(rss, "url");
    if (server_url == NULL) {
        server_url = "http://u.wiidb.de/rssmii/rss_displayer.php";
    }
    printf("Using server URL: %s\n", server_url);*/
    
    for (node = mxmlFindElement(rss, rss, "feed", NULL, NULL, MXML_DESCEND); node != NULL; node = mxmlFindElement(node, rss, "feed", NULL, NULL, MXML_DESCEND)) {
        const char * feedurl = node->child->value.opaque;
        if (feedurl == NULL) {
            return -101;
        }
        
        const char * name = mxmlElementGetAttr(node, "name");
        if (name == NULL) {
            return -102;
        }
        
        int old_length = ijobs;
        RSS_Job * tmp = new RSS_Job[old_length];
        
        // Copy to temp array
        for (int i = 0; i < old_length; i++) {
            tmp[i] = jobs[i];
        }
        
        // Copy back to jobs array
        jobs = new RSS_Job[old_length + 1];
        for (int j = 0; j < old_length; j++) {
            jobs[j] = tmp[j];
        }
        
        memset(rssjob.url, 0, 256);
        memset(rssjob.name, 0, 256);
        snprintf(rssjob.url, 255, feedurl);
        snprintf(rssjob.name, 255, name);
        jobs[old_length] = rssjob;
        ijobs++;
    }
    mxmlDelete(tree);
    return 0;
}

void AddJobs() {
    int which;
    // Initialize WC24
    printf("Init WiiConnect24...\n");
    s32 ret = WC24_Init();
    if (ret < 0) {
        printf("ERROR: WC24_Init returned %d\n", ret);
        return;
    }
    
    // Go through each record with our Title ID and delete it
    printf("Deleting previous entries...\n");
    while( (ret = WC24_FindRecord((u32)rssmiititleid, &nwc24record)) != LIBWC24_ENOENT) {
        WC24_DeleteRecord((u32)ret);
    }
    printf("\n");
    
    for (int i = 0; i < ijobs; i++) {
        printf("Select check interval for \"%s\": (A: 5 min, B: 10 min, 1: 15 min, 2: 30 min, UP: 60 min, RIGHT: 90 min, DOWN: 120 min, LEFT: 180 min)\n", jobs[i].name);
        which = -1;
        while (which == -1) {
            WPAD_ScanPads();
            u32 pressed = WPAD_ButtonsDown(0);
            if (pressed & WPAD_BUTTON_A) which = 5;
            if (pressed & WPAD_BUTTON_B) which = 10;
            if (pressed & WPAD_BUTTON_1) which = 15;
            if (pressed & WPAD_BUTTON_2) which = 30;
            if (pressed & WPAD_BUTTON_UP) which = 60;
            if (pressed & WPAD_BUTTON_RIGHT) which = 90;
            if (pressed & WPAD_BUTTON_DOWN) which = 120;
            if (pressed & WPAD_BUTTON_LEFT) which = 180;
        }
        printf("Creating entry...\n");
        memset(jobs[i].final_url, 0, MAX_BUF + 1);
        int offset = 0;
        offset = snprintf(jobs[i].final_url, MAX_BUF, "http://u.wiidb.de/rssmii/rss_displayer.php?feedurl=%s", url_encode(jobs[i].url, _buffer, MAX_BUF));
        snprintf(jobs[i].final_url + offset, MAX_BUF - offset, "&title=%s", url_encode(jobs[i].name, _buffer, MAX_BUF));
        ret = WC24_CreateRecord(&nwc24record, &nwc24entry, (u32)rssmiititleid, rssmiititleid, 0x4645, WC24_TYPE_MSGBOARD, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_HB, which, 0x5a0, 32767, jobs[i].final_url, NULL);
        if (ret < 0) {
                printf("ERROR: WC24_CreateRecord returned %d\n", ret);
        }
        printf("\n");
    }
    
    for (int i = 0; i < ijobs; i++) {
        printf("Downloading latest entry for \"%s\"...\n", jobs[i].name);

        ret = WC24_FindEntry((u32)rssmiititleid, jobs[i].final_url, &nwc24entry, 0);
        if (ret < 0) {
            printf("FATAL ERROR: Couldn't find entry\n");
        } else {
            u32 ret_dl = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)ret, 0x0);
            if (ret_dl < 0) printf("ERROR while downloading: %d\n", ret_dl);
            
            //Save mail, so the mail content won't get overwritten
            u32 ret_save = KD_SaveMail();
            if (ret_save < 0) printf("ERROR while saving: %d\n", ret_save);
        }
    }
    printf("\n");
    
    // Shutdown WC24
    printf("Shutting down WiiConnect24...\n");
    ret = WC24_Shutdown();
    if (ret < 0) {
        printf("ERROR: WC24_Shutdown returned %d\n", ret);
        exit(0);
    }
}

void fail(int val) {
    printf("ERROR: %i\nPress HOME to exit.\n", val);
    while(1) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) {
            printf("\nExiting...");
            exit(0);
        }
    }
}

int main(int argc, char **argv) {
    // Initialise the video system
    VIDEO_Init();

    // This function initialises the attached controllers
    WPAD_Init();

    // Obtain the preferred video mode from the system
    // This will correspond to the settings in the Wii menu
    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate memory for the display in the uncached region
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialise the console, required for printf
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    // Set up the video registers with the chosen mode
    VIDEO_Configure(rmode);

    // Tell the video hardware where our display memory is
    VIDEO_SetNextFramebuffer(xfb);

    // Make the display visible
    VIDEO_SetBlack(FALSE);

    // Flush the video register changes to the hardware
    VIDEO_Flush();

    // Wait for Video setup to complete
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    printf("\x1b[2;0H");
    printf("Welcome to RSSMii!\n\n");
    
    jobs = new RSS_Job[0];
    int ret = load_feeds();
    if (ret != 0)
    {
        fail(ret);
    }
    
    printf("Subscribe to the following feeds (old ones will be deleted):\n");
    for (int i = 0; i < ijobs; i++) {
        printf("   %s\n", jobs[i].name);
    }
    printf("\nAre you sure? (A: Continue; HOME: Abort)\n");
    
    while (1) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) break;
    }

    AddJobs();
    
    printf("Finished, press HOME to exit!\n");

    while(1) {
        // Call WPAD_ScanPads each loop, this reads the latest controller states
        WPAD_ScanPads();

        // WPAD_ButtonsDown tells us which buttons were pressed in this loop
        // this is a "one shot" state which will not fire again until the button has been released
        u32 pressed = WPAD_ButtonsDown(0);
        
        if (pressed & WPAD_BUTTON_HOME) {
            printf("\nExiting...");
            exit(0);
        }

        // Wait for the next frame
        VIDEO_WaitVSync();
    }

    return 0;
}
