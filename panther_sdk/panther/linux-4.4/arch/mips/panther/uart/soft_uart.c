/* 
 *  Copyright (c) 2017	Montage Technology Group Limited   All rights reserved
 *  
 *  Software UART driver for Panther
 */
#include <linux/types.h>
#include <linux/circ_buf.h>
#include <linux/gpio.h> 
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/ktime.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <linux/console.h>

//#define SOFT_UART_BAUDRATE_19200

#define SOFT_UART_MAJOR            4  // 0
#define SOFT_UART_MINOR            72 // 0
#define N_PORTS                    1
#define NONE                       0

#include <asm/mach-panther/gpio.h>
#include <asm/mach-panther/pinmux.h>

#define CONSOLE_XMIT_BUFFER_SIZE 8192
static unsigned char console_buffer[CONSOLE_XMIT_BUFFER_SIZE];

#define WAKEUP_CHARS 256

static irq_handler_t handle_rx_start(unsigned int irq, void* device, struct pt_regs* registers);
static enum hrtimer_restart handle_tx(struct hrtimer* timer);
static enum hrtimer_restart handle_rx(struct hrtimer* timer);
static void receive_character(unsigned char character);

static DEFINE_SPINLOCK(uart_lock);

static struct circ_buf xmit;
static struct circ_buf console_xmit;
static struct tty_struct* current_tty;
static int current_tty_refcnt;
static struct hrtimer timer_tx;
static struct hrtimer timer_rx;
static ktime_t period;
static int curr_baudrate;
static int gpio_rx_irq = -1;
static int rx_bit_index = -1;

#if defined(CONFIG_PANTHER_TX_GPIO_PIN)
static int gpio_tx = CONFIG_PANTHER_TX_GPIO_PIN;
#else
static int gpio_tx = 15;
#endif

#if defined(CONFIG_PANTHER_RX_GPIO_PIN)
static int gpio_rx = CONFIG_PANTHER_RX_GPIO_PIN;
#else
static int gpio_rx = 16;
#endif

/**
 * Initializes the Raspberry Soft UART infrastructure.
 * This must be called during the module initialization.
 * The GPIO pin used as TX is configured as output.
 * The GPIO pin used as RX is configured as input.
 * @param gpio_tx GPIO pin used as TX
 * @param gpio_rx GPIO pin used as RX
 * @return 1 if the initialization is successful. 0 otherwise.
 */
static int __soft_uart_init(void)
{
    bool success = true;

    if(NULL==timer_tx.function)
    {
        // Initializes the TX timer.
        hrtimer_init(&timer_tx, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        timer_tx.function = &handle_tx;
    }

    // Initializes the RX timer.
    hrtimer_init(&timer_rx, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    timer_rx.function = &handle_rx;

    // Initializes the GPIO pins.
    success &= gpio_request(gpio_tx, "soft_uart_tx") == 0;
    success &= gpio_direction_output(gpio_tx, 1) == 0;

    success &= gpio_request(gpio_rx, "soft_uart_rx") == 0;
    success &= gpio_direction_input(gpio_rx) == 0;

    // Initializes the interruption.
    gpio_rx_irq = gpio_to_irq(gpio_rx);
    success &= request_irq(
                          gpio_rx_irq,
                          (irq_handler_t) handle_rx_start,
                          IRQF_TRIGGER_FALLING,
                          "soft_uart",
                          NULL) == 0;
    disable_irq(gpio_rx_irq);

    pinmux_pin_func_gpio(gpio_tx);
    pinmux_pin_func_gpio(gpio_rx);

    return success;
}

/**
 * Finalizes the Raspberry Soft UART infrastructure.
 */
int soft_uart_finalize(void)
{
    free_irq(gpio_to_irq(gpio_rx), NULL);
    gpio_rx_irq = -1;
    gpio_set_value(gpio_tx, 0);
    gpio_free(gpio_tx);
    gpio_free(gpio_rx);
    return 1;
}

/**
 * Opens the Soft UART.
 * @param tty
 * @return 1 if the operation is successful. 0 otherwise.
 */
int _soft_uart_open(struct tty_struct* tty)
{
    unsigned long flags;
    int success = 0;
    unsigned long page;

    page = get_zeroed_page(GFP_KERNEL);
    if (!page)
        return 0;

    spin_lock_irqsave(&uart_lock, flags);
    current_tty_refcnt++;
    if (current_tty == NULL)
    {
        current_tty = tty;

        if (xmit.buf)
            free_page(page);
        else
            xmit.buf = (unsigned char *) page;

        xmit.head = xmit.tail = 0;

        success = 1;
        enable_irq(gpio_to_irq(gpio_rx));
    }
    else
    {
        success = 1;
        free_page(page);
    }
    spin_unlock_irqrestore(&uart_lock, flags);

    return success;
}

/**
 * Sets the Soft UART baudrate.
 * @param baudrate desired baudrate
 * @return 1 if the operation is successful. 0 otherwise.
 */
int soft_uart_set_baudrate(const int baudrate) 
{
    if (curr_baudrate==baudrate)
        return 1;

    period = ktime_set(0, 1000000000/baudrate);
    curr_baudrate = baudrate;
    return 1;
}

/**
 * Adds a given string to the TX queue.
 * @paran string given string
 * @param string_size size of the given string
 * @return The amount of characters successfully added to the queue.
 */
int soft_uart_send_string(const unsigned char* buf, int count)
{
    int c, ret = 0;
    unsigned long flags;

    if (!xmit.buf)
        return 0;

    spin_lock_irqsave(&uart_lock, flags);
    while (1)
    {
        c = CIRC_SPACE_TO_END(xmit.head,
                              xmit.tail,
                              SERIAL_XMIT_SIZE);
        if (count < c)
            c = count;
        if (c <= 0)
        {
            break;
        }
        memcpy(xmit.buf + xmit.head, buf, c);
        xmit.head = ((xmit.head + c) &
                     (SERIAL_XMIT_SIZE-1));
        buf += c;
        count -= c;
        ret += c;
    }

    if ((xmit.head != xmit.tail) && !hrtimer_active(&timer_tx))
        hrtimer_start(&timer_tx, period, HRTIMER_MODE_REL);

    spin_unlock_irqrestore(&uart_lock, flags);

    return ret;
}

/*
 * Gets the number of characters that can be added to the TX queue.
 * @return number of characters.
 */
int soft_uart_get_tx_queue_room(void)
{
    int room;

    room = CIRC_SPACE(xmit.head, xmit.tail, SERIAL_XMIT_SIZE);

    return room;
}

/*
 * Gets the number of characters in the TX queue.
 * @return number of characters.
 */
int soft_uart_get_tx_queue_size(void)
{
    int size;

    size = CIRC_CNT(xmit.head, xmit.tail, SERIAL_XMIT_SIZE);

    return size;
}

//-----------------------------------------------------------------------------
// Internals
//-----------------------------------------------------------------------------

/**
 * If we are waiting for the RX start bit, then starts the RX timer. Otherwise,
 * does nothing.
 */
static irq_handler_t handle_rx_start(unsigned int irq, void* device, struct pt_regs* registers)
{
    //if (rx_bit_index == -1)
    hrtimer_start(&timer_rx, ktime_set(0, 0), HRTIMER_MODE_REL);

    if (gpio_rx_irq>=0)
        panther_raw_gpio_irq_mask(gpio_rx);

    return(irq_handler_t) IRQ_HANDLED;
}

static int tx_dequeue_char(unsigned char *c, int *con_qsize, int *tty_qsize)
{
    unsigned long flags;
    int console_queue_depth;
    int tty_queue_depth;
    int ret = 0;

    spin_lock_irqsave(&uart_lock, flags);

    console_queue_depth = CIRC_CNT(console_xmit.head, console_xmit.tail, CONSOLE_XMIT_BUFFER_SIZE);
    tty_queue_depth = CIRC_CNT(xmit.head, xmit.tail, SERIAL_XMIT_SIZE);

    if(c)
    {
        if(console_queue_depth)
        {
            *c = console_xmit.buf[console_xmit.tail++];
            console_xmit.tail = console_xmit.tail & (CONSOLE_XMIT_BUFFER_SIZE-1);
            ret = 1;
        }
        else if(tty_queue_depth)
        {
            *c = xmit.buf[xmit.tail++];
            xmit.tail = xmit.tail & (SERIAL_XMIT_SIZE-1);
            ret = 1;
        }
    }

    *con_qsize = console_queue_depth;
    *tty_qsize = tty_queue_depth;

    spin_unlock_irqrestore(&uart_lock, flags);

    return ret;
}

/**
 * Dequeues a character from the TX queue and sends it.
 */
static enum hrtimer_restart handle_tx(struct hrtimer* timer)
{
    enum hrtimer_restart result = HRTIMER_NORESTART;
    bool must_restart_timer = false;

    static ktime_t first_bit_time;
    static ktime_t next_bit_time;
    static unsigned char character;
    static int bit_index = -1;

    ktime_t current_time;

    int tty_queue_size;
    int con_queue_size;

    if (bit_index == -1) // Start bit.
    {
        if(tx_dequeue_char(&character, &con_queue_size, &tty_queue_size))
        {
            camelot_gpio_set_value(gpio_tx, 0);
            current_time = ktime_get();

            first_bit_time = current_time;
            next_bit_time = ktime_add(first_bit_time, period);
            bit_index++;

            must_restart_timer = true;
        }

        if (current_tty && (tty_queue_size < WAKEUP_CHARS))
            tty_wakeup(current_tty);
    } // Data bits.
    else if (0 <= bit_index && bit_index < 8)
    {
        camelot_gpio_set_value(gpio_tx, 1 & (character >> bit_index));
        current_time = ktime_get();

        bit_index++;
        must_restart_timer = true;
        next_bit_time = ktime_add(next_bit_time, period);
    } // Stop bit.
    else
    {
        camelot_gpio_set_value(gpio_tx, 1);

        character = 0;
        bit_index = -1;

        tx_dequeue_char(NULL, &con_queue_size, &tty_queue_size);
        if (con_queue_size || tty_queue_size)
        {
            current_time = ktime_get();
            next_bit_time = ktime_add(next_bit_time, period);
            must_restart_timer = 1;
        }

        if (current_tty && (tty_queue_size < WAKEUP_CHARS))
            tty_wakeup(current_tty);
    }

    // Restarts the TX timer.
    if (must_restart_timer)
    {
        hrtimer_forward(&timer_tx, current_time, 
                        ktime_sub(next_bit_time, current_time));
        result = HRTIMER_RESTART;
    }

    return result;
}

/*
 * Receives a character and sends it to the kernel.
 */
static enum hrtimer_restart handle_rx(struct hrtimer* timer)
{
    bool must_restart_timer = false;

    static unsigned int character = 0;

    ktime_t current_time = ktime_get();
    int bit_value = panther_raw_gpio_get_value(gpio_rx);

    enum hrtimer_restart result = HRTIMER_NORESTART;


    if (rx_bit_index == -1)
    {
        rx_bit_index++;
        character = 0;
        must_restart_timer = true;
    }    // Data bits.
    else if (0 <= rx_bit_index && rx_bit_index < 8)
    {
        if (bit_value == 0)
        {
            character &= 0xfeff;
        } else
        {
            character |= 0x0100;
        }

        rx_bit_index++;
        character >>= 1;
        must_restart_timer = true;
    }    // Stop bit.
    else
    {
        receive_character(character);
        rx_bit_index = -1;

        if (gpio_rx_irq>=0)
            panther_raw_gpio_irq_clear_unmask(gpio_rx);
    }

    // Restarts the RX timer.
    if (must_restart_timer)
    {
        hrtimer_forward(&timer_rx, current_time, period);
        result = HRTIMER_RESTART;
    }

    return result;
}

static int  soft_uart_open(struct tty_struct*, struct file*);
static void soft_uart_close(struct tty_struct*, struct file*);
static int  soft_uart_write(struct tty_struct*, const unsigned char*, int);
static int  soft_uart_write_room(struct tty_struct*);
static void soft_uart_flush_buffer(struct tty_struct*);
static int  soft_uart_chars_in_buffer(struct tty_struct*);
static void soft_uart_set_termios(struct tty_struct*, struct ktermios*);
static void soft_uart_stop(struct tty_struct*);
static void soft_uart_start(struct tty_struct*);
static void soft_uart_hangup(struct tty_struct*);
static int  soft_uart_ioctl(struct tty_struct*, unsigned int, unsigned int long);

// Module operations.
static const struct tty_operations soft_uart_operations = {
    .open            = soft_uart_open,
    .close           = soft_uart_close,
    .write           = soft_uart_write,
    .write_room      = soft_uart_write_room,
    .flush_buffer    = soft_uart_flush_buffer,
    .chars_in_buffer = soft_uart_chars_in_buffer,
    .ioctl           = soft_uart_ioctl,
    .set_termios     = soft_uart_set_termios,
    .stop            = soft_uart_stop,
    .start           = soft_uart_start,
    .hangup          = soft_uart_hangup,
};

// Driver instance.
static struct tty_driver* soft_uart_driver = NULL;
static struct tty_port port;

/**
 * Module initialization.
 */
static int __init soft_uart_init(void)
{
    //printk(KERN_INFO "soft_uart: Initializing module...\n");

    if (!__soft_uart_init())
    {
        printk(KERN_ALERT "soft_uart: Failed initialize GPIO.\n");
        return -ENODEV;
    }

    // Initializes the port.
    tty_port_init(&port);
    port.low_latency = 0;

    // Allocates the driver.
    soft_uart_driver = tty_alloc_driver(N_PORTS, TTY_DRIVER_REAL_RAW);

    // Returns if the allocation fails.
    if (IS_ERR(soft_uart_driver))
    {
        printk(KERN_ALERT "soft_uart: Failed to allocate the driver.\n");
        return -ENOMEM;
    }

    // Initializes the driver.
    soft_uart_driver->owner                 = THIS_MODULE;
    soft_uart_driver->driver_name           = "soft_uart";
    soft_uart_driver->name                  = "ttyG";
    soft_uart_driver->major                 = SOFT_UART_MAJOR;
    soft_uart_driver->minor_start           = SOFT_UART_MINOR;
    soft_uart_driver->flags                 = TTY_DRIVER_REAL_RAW;
    soft_uart_driver->type                  = TTY_DRIVER_TYPE_SERIAL;
    soft_uart_driver->subtype               = SERIAL_TYPE_NORMAL;
    soft_uart_driver->init_termios          = tty_std_termios;

#if defined(SOFT_UART_BAUDRATE_19200)
    soft_uart_driver->init_termios.c_ispeed = 19200;
    soft_uart_driver->init_termios.c_ospeed = 19200;
    soft_uart_driver->init_termios.c_cflag  = B19200 | CREAD | CS8 | CLOCAL | HUPCL;

    soft_uart_set_baudrate(19200);
#else
    soft_uart_driver->init_termios.c_ispeed = 9600;
    soft_uart_driver->init_termios.c_ospeed = 9600;
    soft_uart_driver->init_termios.c_cflag  = B9600 | CREAD | CS8 | CLOCAL | HUPCL;

    soft_uart_set_baudrate(9600);
#endif

    // Sets the callbacks for the driver.
    tty_set_operations(soft_uart_driver, &soft_uart_operations);

    // Link the port with the driver.
    tty_port_link_device(&port, soft_uart_driver, 0);

    // Registers the TTY driver.
    if (tty_register_driver(soft_uart_driver))
    {
        printk(KERN_ALERT "soft_uart: Failed to register the driver.\n");
        put_tty_driver(soft_uart_driver);
        return -ENODEV; // return if registration fails
    }

    //printk(KERN_INFO "soft_uart: Module initialized.\n");
    return 0;
}

/**
 * Cleanup function that gets called when the module is unloaded.
 */
static void __exit soft_uart_exit(void)
{
    //printk(KERN_INFO "soft_uart: Finalizing the module...\n");

    // Finalizes the soft UART.
    soft_uart_finalize();

    // Unregisters the driver.
    if (tty_unregister_driver(soft_uart_driver))
    {
        printk(KERN_ALERT "soft_uart: Failed to unregister the driver.\n");
    }

    put_tty_driver(soft_uart_driver);
    //printk(KERN_INFO "soft_uart: Module finalized.\n");
}

/**
 * Opens a given TTY device.
 * @param tty given TTY device
 * @param file
 * @return error code.
 */
static int soft_uart_open(struct tty_struct* tty, struct file* file)
{
    if(!_soft_uart_open(tty))
    {
        printk(KERN_ALERT "soft_uart: failed to open device.\n");
        return -ENODEV;
    }

    return 0;
}

/**
 * Closes a given TTY device.
 * @param tty
 * @param file
 */
static void soft_uart_close(struct tty_struct* tty, struct file* file)
{
    unsigned long flags;

    spin_lock_irqsave(&uart_lock, flags);

    if(current_tty_refcnt)
    {
        current_tty_refcnt--;

        if(0==current_tty_refcnt)
        {
            disable_irq(gpio_to_irq(gpio_rx));
            xmit.head = xmit.tail = 0;
            current_tty = NULL;

            spin_unlock_irqrestore(&uart_lock, flags);

            hrtimer_cancel(&timer_tx);
            hrtimer_cancel(&timer_rx);

            tty_wakeup(tty);
        }
        else
        {
            spin_unlock_irqrestore(&uart_lock, flags);
        }
    }
    else
    {
        spin_unlock_irqrestore(&uart_lock, flags);
    }
}

/**
 * Writes the contents of a given buffer into a given TTY device.
 * @param tty given TTY device
 * @param buffer given buffer
 * @param buffer_size number of bytes contained in the given buffer
 * @return number of bytes successfuly written into the TTY device
 */
static inline int soft_uart_write(struct tty_struct* tty, const unsigned char* buffer, int buffer_size)
{
    return soft_uart_send_string(buffer, buffer_size);
}

/**
 * Tells the kernel the number of bytes that can be written to a given TTY.
 * @param tty given TTY
 * @return number of bytes
 */
static inline int soft_uart_write_room(struct tty_struct* tty)
{
    return soft_uart_get_tx_queue_room();
}

static void soft_uart_flush_buffer(struct tty_struct* tty)
{
    unsigned long flags;

    spin_lock_irqsave(&uart_lock, flags);
    xmit.head = xmit.tail = 0;
    spin_unlock_irqrestore(&uart_lock, flags);

    tty_wakeup(tty);
}

/**
 * Tells the kernel the number of bytes contained in the buffer of a given TTY.
 * @param tty given TTY
 * @return number of bytes
 */
static inline int soft_uart_chars_in_buffer(struct tty_struct* tty)
{
    return soft_uart_get_tx_queue_size();
}

/**
 * Sets the UART parameters for a given TTY (only the baudrate is taken into account).
 * @param tty given TTY
 * @param termios parameters
 */
static void soft_uart_set_termios(struct tty_struct* tty, struct ktermios* termios)
{
    int cflag = 0;
    speed_t baudrate = tty_get_baud_rate(tty);
    //printk(KERN_INFO "soft_uart: soft_uart_set_termios: baudrate = %d.\n", baudrate);

    // Gets the cflag.
    cflag = tty->termios.c_cflag;

    // Verifies the number of data bits (it must be 8).
    if ((cflag & CSIZE) != CS8)
    {
        printk(KERN_ALERT "soft_uart: Invalid number of data bits.\n");
    }

    // Verifies the number of stop bits (it must be 1).
    if (cflag & CSTOPB)
    {
        printk(KERN_ALERT "soft_uart: Invalid number of stop bits.\n");
    }

    // Verifies the parity (it must be none).
    if (cflag & PARENB)
    {
        printk(KERN_ALERT "soft_uart: Invalid parity.\n");
    }

    // Configure the baudrate.
    if (!soft_uart_set_baudrate(baudrate))
    {
        printk(KERN_ALERT "soft_uart: Invalid baudrate.\n");
    }
}

static void soft_uart_stop(struct tty_struct* tty)
{
    //printk(KERN_DEBUG "soft_uart: soft_uart_stop.\n");
}

static void soft_uart_start(struct tty_struct* tty)
{
    //printk(KERN_DEBUG "soft_uart: soft_uart_start.\n");
}

/**
 * Does nothing.
 * @param tty
 */
static void soft_uart_hangup(struct tty_struct* tty)
{
    //printk(KERN_DEBUG "soft_uart: soft_uart_hangup.\n");

    soft_uart_flush_buffer(tty);

	port.count = 0;
	port.tty = NULL;
	wake_up_interruptible(&port.open_wait);
}

static int soft_uart_ioctl(struct tty_struct* tty, unsigned int command, unsigned int long parameter)
{
    return -ENOIOCTLCMD;
}

// Module entry points.
module_init(soft_uart_init);
module_exit(soft_uart_exit);

/**
 * Adds a given (received) character to the RX buffer, which is managed by the kernel,
 * and then flushes (flip) it.
 * @param character given character
 */
void receive_character(unsigned char character)
{
    if (current_tty != NULL && current_tty->port != NULL)
    {
        tty_insert_flip_char(current_tty->port, character, TTY_NORMAL);
        tty_flip_buffer_push(current_tty->port);
    }
}

#if defined(CONFIG_PANTHER_SOFTWARE_UART_CONSOLE)

static void __serial_console_init(void)
{
    if(NULL==console_xmit.buf)
    {
        current_tty_refcnt++;
        console_xmit.buf = console_buffer;
        console_xmit.head = console_xmit.tail = 0;

        panther_raw_gpio_request(gpio_tx);
        panther_raw_gpio_request(gpio_rx);
        camelot_gpio_direction_output(gpio_tx, 1);
        camelot_gpio_direction_input(gpio_rx);

        pinmux_pin_func_gpio(gpio_tx);
        pinmux_pin_func_gpio(gpio_rx);

#if defined(SOFT_UART_BAUDRATE_19200)
        soft_uart_set_baudrate(19200);
#else
        soft_uart_set_baudrate(9600);
#endif

    }
}

void panther_serial_outc(unsigned char c)
{
    unsigned long flags;

    if(!console_xmit.buf)
        __serial_console_init();

    spin_lock_irqsave(&uart_lock, flags);

    if(0==CIRC_SPACE(console_xmit.head, console_xmit.tail, CONSOLE_XMIT_BUFFER_SIZE))
    {
        /* TODO */
    }
    else
    {
        console_xmit.buf[console_xmit.head++] = c;
        console_xmit.head = console_xmit.head & (CONSOLE_XMIT_BUFFER_SIZE - 1);
    }

    if(timer_tx.function)
    {
        if ((console_xmit.head != console_xmit.tail) && !hrtimer_active(&timer_tx))
            hrtimer_start(&timer_tx, period, HRTIMER_MODE_REL);
    }

    spin_unlock_irqrestore(&uart_lock, flags);
}

static __init int software_uart_console_setup(struct console *co, char *options)
{
    __serial_console_init();

    if(NULL==timer_tx.function)
    {
        hrtimer_init(&timer_tx, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        timer_tx.function = &handle_tx;
    }

    return 0;
}

static void software_uart_console_write(struct console *co, const char *s, unsigned count)
{
    while (count--)
    {
        if (*s == '\n')
            panther_serial_outc('\r');
        panther_serial_outc(*s++);
    }
}

static struct tty_driver *software_uart_console_device(struct console *c, int *index)
{
    *index = 0;
    return soft_uart_driver;
}

static struct console software_uart_console = {
    .name =     "ttyS",
    .write =    software_uart_console_write,
    .device =   software_uart_console_device,
    .setup =    software_uart_console_setup,
    .flags =    CON_PRINTBUFFER,
    .index =    0,
};

/*
 *	Register console.
 */
static int __init software_uart_console_init(void)
{

    register_console(&software_uart_console);

    return 0;
}
console_initcall(software_uart_console_init);

#endif

