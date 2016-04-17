/*
 * OrchIO RF Driver Executable
 *
 * Usage: OrchIO_RFDriver <base_network_id> <base_pipe_id>
 *
 * Where:
 *   base_network_id    4-character string identifying OrchIO Nest
 *   base_pipe_id       1-byte string identifying the first OrchIO pipe served
 * by this RF
 *
 * Description:
 *   The driver will listen to requests on (<base_network_id> << 8 + '?')
 *                                     and (<base_network_id> << 8 +
 * <base_pipe_id>) -- (<base_network_id> << 8 + <base_pipe_id> + 4)
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <inttypes.h>
#include <RF24/RF24.h>

#define VERSION		"1.0.0"

using namespace std;

// Setup RF24 for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
static RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

static uint64_t discoveryAddress;
static uint64_t firstPipeAddress;

void intHandler() {
  bool txok, txfail, rxready;
  uint8_t pipe;
  char receive_payload[64];

  radio.whatHappened(txok, txfail, rxready);

  if (rxready) {
    uint8_t len;

    while (radio.available(&pipe)) {
      len = radio.getDynamicPayloadSize();
      radio.read(receive_payload, len);

      printf("< %02X %02X ", pipe, len);
      for (int i = 0; i < len; i++) {
        printf("%02X", receive_payload[i]);
      }

      cout << '\n';
    }
  }
}

void configure(int argc, char **argv) {
  char nn[5] = "ORCH";
  int firstPipe = 0;
  uint64_t baseAddress;

  if (argc >= 2) {
    if (strlen(argv[1]) >= 4) {
      memcpy(nn, argv[1], 4);
    } else {
      memcpy(nn, argv[1], strlen(argv[1]));
    }
  }

  if (argc >= 3) {
    firstPipe = atoi(argv[2]);
  }

  baseAddress = 0;
  for (int i = 0; i < 4; i++) {
    baseAddress = (baseAddress | nn[i]) << 8;
  }
  discoveryAddress = baseAddress + '?';
  firstPipeAddress = baseAddress + firstPipe;

  fprintf(stderr, "ORCHIO_RFDriver " VERSION "\n");
  fprintf(stderr, "Base address: %" PRIx64 "h\n", baseAddress);
  fprintf(stderr, "Discovery address: %" PRIx64 "h\n", discoveryAddress);
  fprintf(stderr, "First pipe address: %" PRIx64 "h\n", firstPipeAddress);
}

void initialize() {
  radio.begin();
  radio.enableDynamicPayloads();
  radio.setRetries(5, 15);
  radio.setChannel(5);

  attachInterrupt(27, INT_EDGE_FALLING, intHandler);
  radio.openWritingPipe(discoveryAddress);
  radio.openReadingPipe(0, discoveryAddress);
  for (int i = 0; i < 4; i++) {
    radio.openReadingPipe(i + 1, firstPipeAddress + i);
  }
  radio.printDetails();
  radio.startListening();
}

int main(int argc, char **argv) {
  configure(argc, argv);
  initialize();

  /***********************************/

  char ibuffer[256];
  while (fgets(ibuffer, sizeof(ibuffer), stdin)) {
    unsigned int pipe, length;
    char send_payload[64];
    int matched = sscanf(ibuffer, "> %02X %02X ", &pipe, &length);
    if (matched != 2) {
      continue;
    }
    if (pipe < 1 || pipe > 5) {
      continue;
    }
    if (length > sizeof(send_payload)) {
      continue;
    }
    if (strlen(ibuffer) < 9 + length * 2) {
      continue;
    }
    for (unsigned int i = 0; i < length; i++) {
      unsigned int value;
      matched = sscanf(&ibuffer[9 + i * 2], "%02X", &value);
      if (!matched) {
        break;
      }
      send_payload[i] = value;
    }
    if (!matched) {
      continue;
    }

    radio.stopListening();
    radio.openWritingPipe(firstPipeAddress + pipe);
    radio.write(send_payload, length);
    radio.startListening();
  }
}
