/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdint.h>

#include "os.h"
#include "ux.h"

#include "types.h"
#include "globals.h"
#include "ui/menu.h"
#include "io.h"
#include "sw.h"
#include "apdu/parser.h"
#include "apdu/dispatcher.h"

uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
io_state_e io_state;
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

/**
 * Function to handle IO event loop.
 *
 * @brief handle APDU command received and send back APDU response using handlers.
 *
 */
void app_main() {
    // Length of APDU command received in G_io_apdu_buffer
    int input_len = 0;
    // Structured APDU command
    command_t cmd;

    // Reset length of APDU response
    output_len = 0;
    io_state = READY;

    for (;;) {
        BEGIN_TRY {
            TRY {
                // Reset structured APDU command
                os_memset(&cmd, 0, sizeof(cmd));

                // Receive command bytes in G_io_apdu_buffer
                if ((input_len = recv_command()) < 0) {
                    return;
                }

                // Parse APDU command
                if (parse_apdu(&cmd, G_io_apdu_buffer, input_len) < 0) {
                    send_sw(SW_WRONG_DATA_LENGTH);
                    continue;
                }

                // Dispatch APDU command to handler
                if (dispatch_command(&cmd) < 0) {
                    return;
                }
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                send_sw(e);
            }
            FINALLY {
            }
            END_TRY;
        }
    }
}

/**
 * Function to exit the application.
 *
 * @brief exit the application and go back to the dashboard.
 *
 */
void app_exit() {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

/**
 * Entrypoint of the application.
 *
 * @brief main loop to setup USB, Bluetooth, UI and launch app_main().
 *
 */
__attribute__((section(".boot"))) int main() {
    __asm volatile("cpsie i");

    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif  // TARGET_NANOX

                USB_power(0);
                USB_power(1);

                ui_menu_main();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif  // HAVE_BLE

                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                CLOSE_TRY;
                continue;
            }
            CATCH_ALL {
                CLOSE_TRY;
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    app_exit();

    return 0;
}
