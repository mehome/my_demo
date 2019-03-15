#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE3(print, int, level, char __user *, buf, int ,len)
{
	char buf_t[64];
	size_t buf_size;

	buf_size = len > (sizeof(buf_t)-1) ? (sizeof(buf)-1) : len;

	if(copy_from_user(buf_t, buf, len))
		return -EFAULT;

	buf_t[buf_size] = '\0';

	switch(level)
	{
		case LOGLEVEL_EMERG:
			printk(KERN_EMERG "%s", buf_t);
			break;
                case LOGLEVEL_ALERT:
                        printk(KERN_ALERT "%s", buf_t);
                        break;
                case LOGLEVEL_CRIT:
                        printk(KERN_CRIT "%s", buf_t);
                        break;
                case LOGLEVEL_ERR:
                        printk(KERN_ERR "%s", buf_t);
                        break;
                case LOGLEVEL_WARNING:
                        printk(KERN_WARNING "%s", buf_t);
                        break;
                case LOGLEVEL_NOTICE:
                        printk(KERN_NOTICE "%s", buf_t);
                        break;
                case LOGLEVEL_INFO:
                        printk(KERN_INFO "%s", buf_t);
                        break;
                case LOGLEVEL_DEBUG:
                        printk(KERN_DEBUG "%s", buf_t);
                        break;
		default:
			return -EINVAL;
	}

	return len;
}
