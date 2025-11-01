#include <sys/fcntl.h>
#include <sys/ioctl.h>

class c_driver
{
private:
	pid_t pid;

	typedef struct _COPY_MEMORY
	{
		pid_t pid;
		uintptr_t addr;
		void *buffer;
		size_t size;
	} COPY_MEMORY, *PCOPY_MEMORY;

	typedef struct _MODULE_BASE
	{
		pid_t pid;
		char *name;
		uintptr_t base;
	} MODULE_BASE, *PMODULE_BASE;

	enum OPERATIONS
	{
		OP_INIT_KEY = 0x800,
		OP_READ_MEM = 0x801,
		OP_WRITE_MEM = 0x802,
		OP_MODULE_BASE = 0x803,
	};

public:
	c_driver()
	{
		
	}

	~c_driver()
	{
	
	}

	void initialize(pid_t pid)
	{
		this->pid = pid;
	}

	bool read(uintptr_t addr, void *buffer, size_t size)
	{
		COPY_MEMORY cm;

		cm.pid = this->pid;
		cm.addr = addr;
		cm.buffer = buffer;
		cm.size = size;

		if (ioctl(-1, OP_READ_MEM, &cm) != 0)
		{
			return false;
		}
		return true;
	}

	bool write(uintptr_t addr, void *buffer, size_t size)
	{
		COPY_MEMORY cm;

		cm.pid = this->pid;
		cm.addr = addr;
		cm.buffer = buffer;
		cm.size = size;

		if (ioctl(-1, OP_WRITE_MEM, &cm) != 0)
		{
			return false;
		}
		return true;
	}

	template <typename T>
	T read(uintptr_t addr)
	{
		T res;
		if (this->read(addr, &res, sizeof(T)))
			return res;
		return {};
	}

	template <typename T>
	bool write(uintptr_t addr, T value)
	{
		return this->write(addr, &value, sizeof(T));
	}

    pid_t get_name_pid(char *name)
    {
	    FILE *fp;
	    pid_t pid;
	    char cmd[0x100] = "pidof ";

	    strcat(cmd, name);
	    fp = popen(cmd, "r");
	    fscanf(fp, "%d", &pid);
	    pclose(fp);
	    return pid;
    }
    
	uintptr_t get_module_base(char *name)
	{
		MODULE_BASE mb;
		char buf[0x100];
		strcpy(buf, name);
		mb.pid = this->pid;
		mb.name = buf;

		if (ioctl(-1, OP_MODULE_BASE, &mb) != 0)
		{
			return 0;
		}
		return mb.base;
	}
};

static c_driver *driver = new c_driver();
