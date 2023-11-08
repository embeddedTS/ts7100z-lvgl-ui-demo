cmake_minimum_required(VERSION 3.1)
 
project(ts7100z-lvgl-ui-demo)
 
include_directories(.)
 
add_executable(${PROJECT_NAME} gpio.c  gpiolib1.c  main.c  meter.c  TS-7100-Z-Label-20220324.c)
target_link_libraries(${PROJECT_NAME} PRIVATE lvgl lv_drivers iio gpiod input)

install(TARGETS ${PROJECT_NAME})
