# 🐧 Raspberry Pi Linux Device Driver

Raspberry Pi에서 LED와 Push Button을 이용하여
Linux Character Device Driver의 기본 구조부터 Interrupt 기반 Event 처리까지 단계적으로 구현한 실습 프로젝트입니다.

단순 GPIO 제어에서 시작하여 `read()`, `write()`, `ioctl()`, GPIO Interrupt, Wait Queue, Blocking / Non-blocking I/O, `poll()`, `/proc`까지 Linux Device Driver의 주요 기능을 학습하고 구현했습니다.

---

## 📌 Development Environment

* Raspberry Pi 4
* Linux Kernel
* C
* Linux Kernel Module
* GPIO LED / Push Button
* Cross Compilation

---

# 📚 Study Flow

```text
Kernel Module
     ↓
GPIO LED / KEY
     ↓
Character Device Driver
     ↓
read() / write()
     ↓
Major / Minor Number
     ↓
ioctl()
     ↓
GPIO Interrupt
     ↓
kmalloc()
     ↓
filp->private_data
     ↓
Wait Queue
     ↓
Blocking / Non-blocking I/O
     ↓
poll()
     ↓
/proc
```

---

# 1. Linux Kernel Module

Linux Kernel Module의 기본 구조를 학습했습니다.

```c
module_init(driver_init);
module_exit(driver_exit);
```

주요 학습 내용:

* Kernel Module 구조
* `module_init()`
* `module_exit()`
* `insmod`
* `rmmod`
* `lsmod`
* `dmesg`
* Makefile을 이용한 Kernel Module Build

---

# 2. GPIO LED / KEY Control

Raspberry Pi GPIO에 연결된 LED와 Push Button을 Kernel Driver에서 직접 제어했습니다.

```text
Push Button
     ↓
Raspberry Pi GPIO
     ↓
Linux Kernel Driver
     ↓
LED Control
```

주요 내용:

* GPIO 요청 및 해제
* GPIO Input / Output 설정
* LED 출력
* KEY 입력

---

# 3. Character Device Driver

User Space Application에서 `/dev` 파일을 통해 Kernel Driver에 접근할 수 있도록 Character Device Driver를 구현했습니다.

```text
User Application
       │
       │ open / read / write
       ▼
/dev/ledkey_dev
       │
       ▼
Linux Device Driver
       │
       ▼
GPIO LED / KEY
```

주요 구조:

```c
struct file_operations
{
    .owner   = THIS_MODULE,
    .open    = ledkey_open,
    .read    = ledkey_read,
    .write   = ledkey_write,
    .release = ledkey_release,
};
```

학습 내용:

* `struct file_operations`
* `open()`
* `read()`
* `write()`
* `release()`
* `register_chrdev()`
* `unregister_chrdev()`

---

# 4. User Space ↔ Kernel Space Data Transfer

User Space와 Kernel Space는 직접 같은 메모리를 사용할 수 없기 때문에 Kernel API를 이용해 데이터를 전달했습니다.

```text
User Space
     │
     │ write()
     ▼
copy_from_user()
     │
     ▼
Kernel Driver
```

반대 방향:

```text
Kernel Driver
     │
     │ copy_to_user()
     ▼
User Space
```

주요 함수:

```c
copy_to_user();
copy_from_user();
```

---

# 5. Major / Minor Number

Character Device를 구분하기 위해 Major / Minor Number 구조를 학습했습니다.

```text
Device File
   │
   ├── Major Number
   │       └── 어떤 Driver인가
   │
   └── Minor Number
           └── Driver 내부의 어떤 Device인가
```

여러 LED/KEY Device를 하나의 Driver에서 구분하는 구조를 실습했습니다.

---

# 6. ioctl()

`read()` / `write()`만으로 표현하기 어려운 Driver 제어 명령을 전달하기 위해 `ioctl()`을 구현했습니다.

```text
User Application
       │
       │ ioctl(fd, CMD, arg)
       ▼
Kernel Driver
       │
       ▼
unlocked_ioctl()
       │
       ▼
LED / KEY Control
```

주요 학습 내용:

* `unlocked_ioctl`
* ioctl Command 정의
* User / Kernel 공통 Header
* Command와 Data 분리

---

# 7. GPIO Interrupt

Push Button 상태를 반복해서 확인하는 방식 대신 GPIO Interrupt를 이용하여 Event 기반 구조를 구현했습니다.

```text
KEY Input
    ↓
GPIO Edge
    ↓
Hardware Interrupt
    ↓
Linux IRQ
    ↓
ISR
    ↓
Event Processing
```

주요 Kernel API:

```c
gpio_to_irq();
request_irq();
free_irq();
```

상승 / 하강 Edge Interrupt를 이용하여 KEY 입력 상태 변화를 처리했습니다.

---

# 8. Dynamic Memory Allocation

Driver 내부 데이터를 동적으로 관리하기 위해 `kmalloc()`을 사용했습니다.

```text
open()
  │
  ▼
kmalloc()
  │
  ▼
Driver Data Structure
  │
  ▼
release()
  │
  ▼
kfree()
```

주요 내용:

```c
kmalloc();
kfree();
```

---

# 9. filp->private_data

`open()`에서 생성한 Driver별 데이터를 `read()`, `write()`, `release()` 등의 함수에서도 사용할 수 있도록 `filp->private_data`를 활용했습니다.

```text
open()
   │
   ▼
kmalloc()
   │
   ▼
pkeyData
   │
   ├─────────────┐
   ▼             ▼
filp->private_data   request_irq()
   │                 │
   ▼                 ▼
read() / write()     ISR
```

이를 통해 파일을 연 Process별 Driver Context를 관리하는 방법을 학습했습니다.

---

# 10. Wait Queue

Interrupt가 발생하기 전까지 User Process를 Sleep 상태로 두기 위해 Wait Queue를 적용했습니다.

```text
Application
    │
    │ read()
    ▼
Driver
    │
    │ 데이터 없음
    ▼
Wait Queue
    │
    │ Process Sleep
    │
    │ ← KEY Interrupt
    ▼
wake_up_interruptible()
    │
    ▼
read() Resume
```

주요 API:

```c
DECLARE_WAIT_QUEUE_HEAD();
wait_event_interruptible();
wake_up_interruptible();
```

---

# 11. Blocking / Non-blocking I/O

Application의 `open()` Flag에 따라 Driver의 동작을 다르게 구현했습니다.

## Blocking I/O

데이터가 없으면 Process가 Wait Queue에서 대기합니다.

```text
read()
  ↓
No Event
  ↓
Sleep
  ↓
Interrupt
  ↓
Wake Up
  ↓
Data Return
```

## Non-blocking I/O

데이터가 없으면 즉시 반환합니다.

```c
if (filp->f_flags & O_NONBLOCK)
    return -EAGAIN;
```

이를 통해 Blocking과 Non-blocking I/O의 차이를 학습했습니다.

---

# 12. poll()

여러 File Descriptor의 Event를 효율적으로 기다릴 수 있도록 Driver에 `poll()` Interface를 추가했습니다.

```text
User Application
      │
      │ poll()
      ▼
Driver poll()
      │
      ▼
poll_wait()
      │
      ▼
Wait Queue
      │
      │ KEY Interrupt
      ▼
Wake Up
      │
      ▼
POLLIN
      │
      ▼
Application read()
```

Driver:

```c
poll_wait(filp, &wait_queue, wait);

if (event)
    mask |= POLLIN | POLLRDNORM;
```

Application:

```c
poll(&pfd, 1, -1);
```

`poll()`은 GPIO 값을 반복해서 검사하는 Busy Polling과 다르게 Event가 발생할 때까지 Process를 Sleep 상태로 둘 수 있습니다.

---

# 13. /proc Interface

Kernel 내부의 상태를 User Space에서 확인하기 위해 `/proc` Virtual File System을 활용했습니다.

```text
User Space
     │
     │ read()
     ▼
/proc/ledkey
     │
     ▼
Linux Kernel
     │
     ▼
Driver Status
```

`/proc`의 파일은 실제 Storage에 존재하는 일반 파일이 아니라 Kernel이 요청 시 데이터를 생성하는 Virtual File입니다.

학습 내용:

* procfs
* Kernel State 확인
* User Space에서 Driver 정보 조회

---

# 🔄 Final Event Flow

최종적으로 LED/KEY Driver를 다음과 같은 Event 기반 구조로 구현했습니다.

```text
Push Button
     │
     ▼
GPIO Interrupt
     │
     ▼
ISR
     │
     ├── Key Data Update
     │
     ▼
Wake Queue
     │
     ▼
poll()
     │
     ▼
read()
     │
     ▼
User Application
     │
     ▼
LED Control
```

---

# 📁 Repository Structure

```text
raspberrypi-linux-device-driver/
│
├── README.md
│
├── 01_kernel_module/
├── 02_gpio_ledkey/
├── 03_character_device/
├── 04_major_minor/
├── 05_ioctl/
├── 06_interrupt/
├── 07_kmalloc_private_data/
├── 08_blocking_io/
├── 09_poll/
└── 10_proc/
```

---

# 🔑 Keywords

`Linux Kernel` `Device Driver` `Raspberry Pi` `Character Device` `GPIO` `Interrupt` `Wait Queue` `Blocking I/O` `Non-blocking I/O` `poll()` `ioctl()` `kmalloc()` `private_data` `procfs`
