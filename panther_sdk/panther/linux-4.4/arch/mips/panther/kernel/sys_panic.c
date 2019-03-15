#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(panic, char __user *, buf, int ,len)
{
	char buf_t[64];
	size_t buf_size;

	buf_size = len > (sizeof(buf_t)-1) ? (sizeof(buf)-1) : len;

	if(copy_from_user(buf_t, buf, len))
		return -EFAULT;

	buf_t[buf_size] = '\0';

	panic("%s", buf_t);

	return len;
}
