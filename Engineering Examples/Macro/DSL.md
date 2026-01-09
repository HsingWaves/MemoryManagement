Linux kernel:

```
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member)              \
    for (pos = list_first_entry(head, typeof(*pos), member); \
         &pos->member != (head);                            \
         pos = list_next_entry(pos, member))

```

```
struct task_struct *task;
list_for_each_entry(task, &task_list, tasks) {
    printk("%d\n", task->pid);
}

```

Qemu:

```
OBJECT_DEFINE_TYPE(MyDevice, my_device, MY_DEVICE, TYPE_DEVICE)

struct MyDevice {
    DeviceState parent_obj;
    int foo;
};

```

```
static void my_device_class_init(ObjectClass *klass, void *data);
static void my_device_init(MyDevice *obj);

TypeInfo my_device_info = {
    .name = TYPE_MY_DEVICE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(MyDevice),
    .class_init = my_device_class_init,
    .instance_init = my_device_init,
};

```

SQLite

```
#define WHERE_BEGIN ...
#define WHERE_END ...
#define WHERE_COLUMN_EQ(col, val) ...

```

```
WHERE_BEGIN;
WHERE_COLUMN_EQ(id, 42);
WHERE_END;

```

C has no generics/templates

Performance & ABI stability are priorities

The project is very large (Linux/QEMU)

C++/Rust (history + toolchain) is not allowed