#include <string.h>

#define MAX_MSG_LEN 256
#define BLE_CMD_LEN 5
#define BLE_ADDR_LEN 2
#define ESCAPE_BYTE 0xFA
#define UART_START 0xFF
#define UART_END 0xFE

/* UART Base function, encode and decode uart message*/
// log to Serial port that's connected with usb
void uart_log_encoded_bytes(byte* ble_cmd, size_t ble_cmd_len, byte* data_buffer, size_t data_length) {
  byte esacpe_byte = ESCAPE_BYTE;
  byte uart_start = UART_START;
  byte uart_end = UART_END;
  
  Serial.write(&uart_start, 1);

  // encode ble_cmd
  for (byte* byte_itr = ble_cmd ; byte_itr < ble_cmd + ble_cmd_len; ++byte_itr) {
    if (byte_itr[0] < esacpe_byte) {
      Serial.write(byte_itr, 1);
      continue;
    }

    // need 2 byte encoded
    byte encoded = byte_itr[0] ^ esacpe_byte; // bitwise Xor
    Serial.write(&esacpe_byte, 1);
    Serial.write(&encoded, 1);
  }
  
  // encode data_buffer
  for (byte* byte_itr = data_buffer ; byte_itr < data_buffer + data_length; ++byte_itr) {
    if (byte_itr[0] < esacpe_byte) {
      Serial.write(byte_itr, 1);
      continue;
    }

    // need 2 byte encoded
    byte encoded = byte_itr[0] ^ esacpe_byte; // bitwise Xor
    Serial.write(&esacpe_byte, 1);
    Serial.write(&encoded, 1);
  }

  Serial.write(&uart_end, 1);
  Serial.println();
}

// write to Serial1 port that's connected tx/rx pin
void uart_write_encoded_bytes(byte* ble_cmd, size_t ble_cmd_len, byte* data_buffer, size_t data_length) {
  byte esacpe_byte = ESCAPE_BYTE;
  byte uart_start = UART_START;
  byte uart_end = UART_END;
  
  Serial1.write(&uart_start, 1);

  // encode ble_cmd
  for (byte* byte_itr = ble_cmd ; byte_itr < ble_cmd + ble_cmd_len; ++byte_itr) {
    if (byte_itr[0] < esacpe_byte) {
      Serial1.write(byte_itr, 1);
      continue;
    }

    // need 2 byte encoded
    byte encoded = byte_itr[0] ^ esacpe_byte; // bitwise Xor
    Serial1.write(&esacpe_byte, 1);
    Serial1.write(&encoded, 1);
  }
  
  // encode data_buffer
  for (byte* byte_itr = data_buffer ; byte_itr < data_buffer + data_length; ++byte_itr) {
    if (byte_itr[0] < esacpe_byte) {
      Serial1.write(byte_itr, 1);
      continue;
    }

    // need 2 byte encoded
    byte encoded = byte_itr[0] ^ esacpe_byte; // bitwise Xor
    Serial1.write(&esacpe_byte, 1);
    Serial1.write(&encoded, 1);
  }

  Serial1.write(&uart_end, 1);
}

// Read and Decode uart message
size_t uart_readAndDecode_message(class Uart serial_port, byte* data_buffer, size_t buffer_len, size_t* data_len_ptr) {
  size_t byte_read = 0;
  size_t data_len = 0;
  
  // locate uart start
  while (serial_port.available() > 0) {
    byte data = serial_port.read();
    if (data == UART_START) {
      break;
    }
  }

  // read untile uart end
  while (serial_port.available() > 0) {
    byte data = serial_port.read();
    if (data == UART_END) {
      break;
    }

    if (data == ESCAPE_BYTE) {
      byte encoded = serial_port.read();
      data = encoded ^ ESCAPE_BYTE;
      byte_read += 1;
    }

    // store this byte to data_buffer
    data_buffer[data_len] = data;
    byte_read += 1;
    data_len += 1;
  }

  if (data_len > buffer_len) {
    Serial.printf("[Error] Read %d byte decoed to %d byte > %d byte buffer length !!", byte_read, data_len, buffer_len);
  }

  *data_len_ptr = data_len;


  // Debug log
  Serial.print("[UART] >> \'");
  Serial.write(data_buffer, byte_read);
  Serial.println("\'");
}

/* Example Function, Send Data update Network Server */
void ble_send_to_root(byte* data_buffer, size_t data_length) {
  // SEND- command need 8 byte for command meta
  // SEND-|2_byte_addr|1_byte_msg_len|message
  byte ble_cmd[8] = "SEND-";
  
  // 2 byte dst_addr, 0 for root
  ble_cmd[5] = 0x00; 
  ble_cmd[6] = 0x00; 

  // write length
  ble_cmd[7] = (byte) data_length;
  uart_write_encoded_bytes(ble_cmd, 8, data_buffer, data_length);
  // uart_log_encoded_bytes(ble_cmd, 8, data_buffer, data_length);
}

void sendGPS(int p1, int p2, int p3) {
  // [D]|size_n|data_type|data_length_byte|data|...|data_type|data_length_byte|data|[E]
  byte buffer[MAX_MSG_LEN];
  byte* buf_itr = buffer;

  // message type
  strncpy((char*)buf_itr, "[D]", 3);
  buf_itr +=3;

  // 1 byte size_n of data amount, only one for GPS
  buf_itr[0] = 0x01;
  buf_itr += 1;

  // data type - 3 byte
  strncpy((char*)buf_itr, "GPS", 3);
  buf_itr +=3;

  // data len - 1 byte
  buf_itr[0] = 0x06; // 6 byte GPS data
  buf_itr += 1;

  // fake temp GPS Data - 6 byte
  buf_itr[0] = 0xff;
  buf_itr[1] = 0xff;
  buf_itr[2] = 0xff;
  buf_itr[3] = 0xff;
  buf_itr[4] = 0xff;
  buf_itr[5] = 0xff;
  buf_itr += 6;
  
  // end of message
  strncpy((char*)buf_itr, "[E]", 3);
  buf_itr +=3;

  ble_send_to_root(buffer, buf_itr - buffer);
}

void sendTestMutipleData(int16_t *fake_gps, int16_t *fake_ldc, int8_t *fake_idx) {
  // [D]|size_n|data_type|data_length_byte|data|...|data_type|data_length_byte|data|[E]
  byte buffer[MAX_MSG_LEN];
  byte* buf_itr = buffer;

  // message type
  strncpy((char*)buf_itr, "[D]", 3);
  buf_itr +=3;

  // 1 byte size_n of data amount
  buf_itr[0] = 0x03;
  buf_itr += 1;

  // ------------ data ---------------- 
  // data type - 3 byte
  strncpy((char*)buf_itr, "GPS", 3);
  buf_itr +=3;

  // data len - 1 byte
  buf_itr[0] = 0x06; // 6 byte GPS data
  buf_itr += 1;

  // fake temp GPS Data - 6 byte
  memcpy(buf_itr, fake_gps, 2);
  buf_itr += 2;
  memcpy(buf_itr, fake_gps, 2);
  buf_itr += 2;
  memcpy(buf_itr, fake_gps, 2);
  buf_itr += 2;
  
  // ------------ data ---------------- 
  // data type - 3 byte
  strncpy((char*)buf_itr, "LDC", 3);
  buf_itr +=3;

  // data len - 1 byte
  buf_itr[0] = 0x02; // 2 byte GPS data
  buf_itr += 1;

  // fake temp LDC Data - 2 byte
  memcpy(buf_itr, fake_ldc, 2);
  buf_itr += 2;

  // ------------ data ---------------- 
  // data type - 3 byte
  strncpy((char*)buf_itr, "IDX", 3);
  buf_itr +=3;

  // data len - 1 byte
  buf_itr[0] = 0x01;
  buf_itr += 1;

  // fake temp Data - 1 byte
  memcpy(buf_itr, fake_idx, 1);
  buf_itr += 1;

  // ------------ encode test \xfb data ---------------- 
  // data type - 3 byte
  strncpy((char*)buf_itr, "ESP", 3);
  buf_itr +=3;

  // data len - 1 byte
  buf_itr[0] = 0x04;
  buf_itr += 1;

  // fake temp Data - 1 byte
  buf_itr[0] = 0xfa;
  buf_itr += 1;
  buf_itr[0] = 0xfb;
  buf_itr += 1;
  buf_itr[0] = 0xfc;
  buf_itr += 1;
  buf_itr[0] = 0xfd;
  buf_itr += 1;

  ble_send_to_root(buffer, buf_itr - buffer);
}

/* Example Function, Send Request to Network Server */
void sendRobotRequest() {
  
}

void setup() {
  Serial.begin(115200); // usb monitor
  // while (!Serial);
  Serial1.begin(115200); // tx-pin6 rx-pin7
  while (!Serial1);
}

int16_t fake_gps = 64000;
int16_t fake_lds = 59910;
int8_t fake_idx = 0;

void loop() {
  sendTestMutipleData(&fake_gps, &fake_lds, &fake_idx);
  fake_gps += 20;
  fake_lds += 10;
  fake_idx += 1;
  byte data[1024];
  size_t data_len;
  uart_readAndDecode_message(Serial1, data, 1024, &data_len);
  uart_readAndDecode_message(Serial1, data, 1024, &data_len);
  delay(5000);
}
