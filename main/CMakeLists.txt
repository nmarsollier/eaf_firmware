# main component — sources and REQUIRES grouped by module.
set(main_srcs
        "main.c"

        "led/led_init.c"
        "led/led_state.c"
        "led/led_beacon.c"
        "led/led_update.c"

        "tmc/tmc_init.c"
        "tmc/tmc_get_initialized.c"
        "tmc/tmc_status.c"

        "motors/motors_init.c"
        "motors/motors_hw.c"
        "motors/motors_rmt.c"
        "motors/motors_command_queue.c"
        "motors/motors_motion_task.c"
        "motors/motors_current_state.c"
        "motors/motors_error.c"
        "motors/motors_power.c"
        "motors/motors_move.c"
        "motors/motors_stop.c"
        "motors/motors_enable.c"

        "buttons/buttons.c"

        "runtime/runtime_setup.c"
        "runtime/runtime_loop.c"
)

# USB transport + protocol: built only when TinyUSB CDC is enabled. The debug
# build (TinyUSB disabled) uses the USB-Serial/JTAG console instead.
if(CONFIG_TINYUSB_CDC_ENABLED)
        list(APPEND main_srcs
                "usb_cdc/usb_cdc_init.c"
                "usb_cdc/usb_cdc_rx.c"
                "usb_cdc/usb_cdc_tx.c"

                "proto/proto_init.c"
                "proto/proto_frame.c"
                "proto/proto_json.c"
                "proto/proto_dispatch.c"
                "proto/proto_session.c"
                "proto/proto_operation.c"
                "proto/proto_capabilities.c"
                "proto/proto_state.c"
                "proto/proto_action.c"
                "proto/proto_config.c"
                "proto/proto_control.c"
        )
endif()

idf_component_register(
        SRCS ${main_srcs}

        INCLUDE_DIRS
        "."
        "config"
        "led"
        "tmc"
        "motors"
        "usb_cdc"
        "proto"
        "buttons"
        "runtime"

        REQUIRES
        esp_timer
        nvs_flash
        esp_driver_gpio
        esp_driver_uart
        esp_driver_rmt
)
