#include <wchar.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>

#define VENDOR_ID  0x2573
#define PRODUCT_ID 0x0017

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
         _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
         _a < _b ? _a : _b; })

void enumerate_hid()
{
    hid_device_info * hinfo;
    hid_device_info * enumerate;

    enumerate = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    if(enumerate != NULL)
    {
        hinfo = enumerate;

        while(hinfo != NULL)
        {
            wprintf(L"%04x:%04x - %s %ls\n", hinfo->vendor_id, hinfo->product_id, hinfo->path, hinfo->serial_number);
            wprintf(L"    vendor: %ls\n", hinfo->manufacturer_string);
            wprintf(L"    product: %ls\n", hinfo->product_string);

            hinfo = hinfo->next;
        }

        hid_free_enumeration(enumerate);
    }
    else
        wprintf(L"HID dev list is empty\n");
}

int send(hid_device *dev, unsigned char op, unsigned char byte)
{
    unsigned char buf[] = {0x00,
        0x12, 0x34, op,   0x00, 0x01,
        byte, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00,
        0x00, 0x00
    };
    
    int res = hid_write(dev, buf, sizeof(buf));

    if( res < 0 )
        wprintf(L"ERROR: Operation failed: %d\n", res);

    return res;
}

int main(int argc, char *argv[])
{
    // Options values
    long input_l = 190, input_r = 190, output_l = 255, output_r = 255;
    bool monitor = false, input_mute = false;
    unsigned char input_channel = 0x8; // MIC: 0x1, HIZ: 0x2, LINE: 0x4, MIC_HIZ: 0x8, MUTE: 0xc2

    bool do_e = false, do_c = false, do_m = false, do_l = false, do_r = false, do_L = false, do_R = false;

    // Parse arguments
    int opt = 0;
    char c;
    while( (c = getopt (argc, argv, "edc:MmIil:r:L:R:")) != -1 ) {
        switch( c ) {
            case 'e':
                do_e = true;
                break;
            case 'd':
                do_c = true; do_m = true; do_l = true; do_r = true; do_L = true; do_R = true;
                break;
            case 'c':
                do_c = true;
                if( optarg[0] == 'm' ) {
                    if( optarg[strlen(optarg)-1] == 'z' )
                        input_channel = 0x8;
                    else if( optarg[strlen(optarg)-1] == 'e' )
                        input_channel = 0xc2;
                    else
                        input_channel = 0x1;
                }
                else if( optarg[0] == 'h' )
                    input_channel = 0x2;
                else if( optarg[0] == 'l' )
                    input_channel = 0x4;
                else
                    do_c = false;
                break;
            case 'M':
            case 'm':
                do_m = true;
                monitor = c == 'M';
                break;
            case 'l':
                do_l = true;
                input_l = atoi(optarg);
                input_l = max(min(input_l, 255), 0);
                break;
            case 'r':
                do_r = true;
                input_r = atoi(optarg);
                input_r = max(min(input_r, 255), 0);
                break;
            case 'L':
                do_L = true;
                output_l = atoi(optarg);
                output_l = max(min(output_l, 255), 0);
                break;
            case 'R':
                do_R = true;
                output_r = atoi(optarg);
                output_r = max(min(output_r, 255), 0);
                break;
            default:
                wprintf(L"Usage: %s [options]\n\n", argv[0]);
                wprintf(L"  -e          - Enumerate available devices\n");
                wprintf(L"  -d          - Set default values\n");
                wprintf(L"  -c <name>   - Set input channel ('mic', 'hiz', 'line', 'mic_hiz', 'mute')\n");
                wprintf(L"  -M          - Input monitoring on\n");
                wprintf(L"  -m          - Input monitoring off\n");
                wprintf(L"  -l <0-255>  - Input left volume\n");
                wprintf(L"  -r <0-255>  - Input right volume\n");
                wprintf(L"  -L <0-255>  - Output left volume\n");
                wprintf(L"  -R <0-255>  - Output right volume\n");
                exit(0);
        }
    }

    // Connecting to device
    hid_device *hiddev;

    if( hid_init() != 0 )
        return -1;

    if( do_e )
        enumerate_hid();

    if( do_c || do_m || do_l || do_r || do_L || do_R ) {
        hiddev = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
        if( hiddev != NULL ) {
            if( do_c ) {
                wprintf(L"  Set input channel: %d\n", input_channel);
                send(hiddev, 0x2a, input_channel);
            }
            if( do_m ) {
                wprintf(L"  Set monitor: %s\n", monitor ? "enable": "disable");
                send(hiddev, 0x2c, monitor ? 0x05 : 0x01);
            }
            if( do_l ) {
                wprintf(L"  Set input left volume: %d\n", input_l);
                send(hiddev, 0x1c, input_l);
            }
            if( do_r ) {
                wprintf(L"  Set input right volume: %d\n", input_r);
                send(hiddev, 0x1e, input_r);
            }
            if( do_L ) {
                wprintf(L"  Set output left volume: %d\n", output_l);
                send(hiddev, 0x07, output_l);
            }
            if( do_R ) {
                wprintf(L"  Set output right volume: %d\n", output_r);
                send(hiddev, 0x09, output_r);
            }
            hid_close(hiddev);
        } else
            wprintf(L"Unable to open hid device\n");
    }

    hid_exit();

    return 0;
}