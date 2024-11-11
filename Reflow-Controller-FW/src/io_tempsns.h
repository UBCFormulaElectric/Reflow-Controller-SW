#pragma once

enum read_temp_status {
    READ_TEMP_OK,
    READ_TEMP_FAIL
};

struct temp_read_res {
    enum read_temp_status status;
    double value;
};
/**
* Requests a read from the temperature sensor IC
* @returns the status of the read, and the value it read
*/
struct temp_read_res io_tempsns_read();
