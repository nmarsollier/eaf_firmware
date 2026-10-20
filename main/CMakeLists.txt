# main component — sources and REQUIRES grouped by module.
idf_component_register(
        SRCS
        "main.c"

        "board/board.c"

        "led/led_init.c"
        "led/led_state.c"
        "led/led_beacon.c"
        "led/led_update.c"

        "i2c_bus/i2c_bus.c"

        "accelerometer/accelerometer_init.c"
        "accelerometer/accelerometer_read.c"

        "sht31/sht31_init.c"
        "sht31/sht31_read.c"

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

        "fan/fan.c"
        "environment/environment.c"
        "vibration/vibration.c"
        "buttons/buttons.c"

        "runtime/runtime_setup.c"
        "runtime/runtime_loop.c"

        INCLUDE_DIRS
        "."
        "config"
        "i2c_bus"
        "accelerometer"
        "sht31"
        "board"
        "led"
        "tmc"
        "motors"
        "usb_cdc"
        "proto"
        "fan"
        "environment"
        "vibration"
        "buttons"
        "runtime"

        REQUIRES
        esp_timer
        nvs_flash
        esp_driver_gpio
        esp_driver_ledc
        esp_adc
        esp_driver_uart
        esp_driver_rmt
        esp_driver_i2c
)
