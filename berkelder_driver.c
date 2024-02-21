#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Larissa Berkelder");
MODULE_DESCRIPTION("Device driver OS2");

#define DEVICE_NAME "berkelder-driver"
static int major;

// 7 segment display number of GPIO pins
#define NUM_GPIO_PINS 8
static int gpio_pins[NUM_GPIO_PINS] = {17, 27, 6, 5, 22, 23, 24 ,26};
static char *gpio_pin_names[NUM_GPIO_PINS] = {"A", "B", "C", "D", "E", "F", "G", "DP"};
static int segment_value[10][NUM_GPIO_PINS] = {
        //a  b  c  d  e  f  g  dp
        {0, 0, 0, 0, 0, 0, 1, 1}, // 0
        {1, 0, 0, 1, 1, 1, 1, 1}, // 1
        {0, 0, 1, 0, 0, 1, 0, 1}, // 2
        {0, 0, 0, 0, 1, 1, 0, 1}, // 3
        {1, 0, 0, 1, 1, 0, 0, 1}, // 4
        {0, 1, 0, 0, 1, 0, 0, 1}, // 5
        {0, 1, 0, 0, 0, 0, 0, 1}, // 6
        {0, 0, 0, 1, 1, 1, 1, 1}, // 7
        {0, 0, 0, 0, 0, 0, 0, 1}, // 8
        {0, 0, 0, 0, 1, 0, 0, 1}, // 9
};

static char buffer[255];
static int buffer_pointer = 0;

static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs){
    int to_copy, not_copied, delta;
    printk("berkelder-driver - read was called\n");
    if(*offs >= buffer_pointer){
        return delta;
    }
    to_copy = min(count, buffer_pointer - (int)*offs);
    not_copied = copy_to_user(user_buffer, buffer + *offs, to_copy);
    delta = to_copy - not_copied;
    *offs += delta;
    return delta;
}

static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs){
    int to_copy, not_copied, delta;
    printk("berkelder-driver - write was called\n");

    memset(buffer, 0, sizeof(buffer));

    to_copy = min(count, sizeof(buffer) - 1);
    not_copied = copy_from_user(buffer, user_buffer, to_copy);
    buffer[to_copy - not_copied] = '\0';

    delta = to_copy - not_copied;

    int num;
    if(sscanf(buffer, "on %d", &num) == 1) {
        if(num >= 0 && num <= 9) {
            // Turn on the segments for the given number
            for (int i = 0; i < NUM_GPIO_PINS; i++) {
                gpio_set_value(gpio_pins[i], segment_value[num][i]);
            }
            printk(KERN_INFO "berkelder-driver - Displaying number %d\n", num);
        } else {
            printk(KERN_INFO "berkelder-driver - Number %d is out of range\n", num);
        }
    }
    else if (strncmp(buffer, "off", 3) == 0) {
        printk(KERN_INFO "Turning 7 segment off\n");
        int i;
        for(i = 0; i < NUM_GPIO_PINS; i++) {
            gpio_set_value(gpio_pins[i], 1); // Set each pin to high to turn pins off (Anode)
        }
    }
    else {
        printk(KERN_INFO "berkelder-driver - Unknown command\n");
    }

    buffer_pointer = to_copy;
    return delta;
}

static int driver_open(struct inode *device_file, struct file *instance) {
    printk("berkelder-driver - open was called!\n");
    return 0;
}

static int driver_release(struct inode *device_file, struct file *instance) {
    printk("berkelder-driver - close was called!\n");
    return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = driver_open,
        .release = driver_release,
        .read = driver_read,
        .write = driver_write
};

static int my_init(void) {
    printk(KERN_INFO "Hello, initializing!\n");
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if(major < 0){
        printk(KERN_ALERT "Registering char device failed with %d\n", major);
        return major;
    }
    printk(KERN_INFO "Registered the character device driver with major number %d\n", major);
    int i;
    // Initialize GPIO pins for the 7-segment display
    for (i = 0; i < NUM_GPIO_PINS; i++) {
        if (gpio_request(gpio_pins[i], gpio_pin_names[i])) {
            printk(KERN_ALERT "Cannot allocate GPIO %d\n", gpio_pins[i]);
            goto GpioError;
        }
        if (gpio_direction_output(gpio_pins[i], 0)) {
            printk(KERN_ALERT "Cannot set GPIO %d to output\n", gpio_pins[i]);
            goto GpioError;
        }
    }

    return 0;

    GpioError:
    // Free any GPIOs that were successfully requested before the error
    while (i-- > 0) {
        gpio_free(gpio_pins[i]);
    }
    unregister_chrdev(major, DEVICE_NAME);
    return -1;
}

static void my_exit(void) {
    unregister_chrdev(major, DEVICE_NAME);
    for (int i = 0; i < NUM_GPIO_PINS; ++i) {
        gpio_free(gpio_pins[i]);
    }
    printk("Bye, exiting\n");
}

module_init(my_init);
module_exit(my_exit);