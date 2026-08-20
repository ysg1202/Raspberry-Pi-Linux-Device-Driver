# 🐧 Raspberry Pi Linux Device Driver

Raspberry Pi에서 LED와 Push Button을 이용한 Linux Character Device Driver 실습

GPIO 제어부터 `read()`, `write()`, `ioctl()`, GPIO Interrupt, Wait Queue, Blocking / Non-blocking I/O, `poll()`, `/proc`까지 단계별 구성

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

Kernel에 동적으로 추가하거나 제거할 수 있는 모듈 구조

```c
module_init(driver_init);
module_exit(driver_exit);
```

주요 명령어

* `insmod` : 모듈 적재
* `rmmod` : 모듈 제거
* `lsmod` : 로드된 모듈 확인
* `dmesg` : 커널 로그 확인

---

# 2. GPIO LED / KEY Control

GPIO를 이용한 LED 출력 및 Push Button 입력 처리

```text
Push Button
     ↓
Raspberry Pi GPIO
     ↓
Linux Kernel Driver
     ↓
LED Control
```

주요 내용

* GPIO 요청 / 해제
* GPIO Input / Output 설정
* LED 출력
* KEY 입력

---

# 3. Character Device Driver

User Space에서 `/dev` 파일을 통해 Kernel Driver에 접근하는 구조

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

주요 함수

* `open()`
* `read()`
* `write()`
* `release()`
* `register_chrdev()`
* `unregister_chrdev()`

---

# 4. User Space ↔ Kernel Space Data Transfer

User Space와 Kernel Space 사이의 데이터 전달

### User → Kernel

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

### Kernel → User

```text
Kernel Driver
     │
     │ copy_to_user()
     ▼
User Space
```

주요 API

```c
copy_to_user();
copy_from_user();
```

---

# 5. Major / Minor Number

Character Device 구분에 사용하는 번호

```text
Device File
   │
   ├── Major Number
   │       └── Driver 구분
   │
   └── Minor Number
           └── Driver 내부 Device 구분
```

하나의 Driver에서 여러 Device 구분 가능

---

# 6. ioctl()

`read()` / `write()` 외의 별도 제어 명령 전달

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

주요 내용

* `unlocked_ioctl`
* ioctl Command 정의
* User / Kernel 공통 Header
* Command / Data 분리

---

# 7. GPIO Interrupt

Push Button 상태 변화를 GPIO Interrupt로 처리

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

주요 API

```c
gpio_to_irq();
request_irq();
free_irq();
```

Rising / Falling Edge 기반 Interrupt 처리

---

# 8. Dynamic Memory Allocation

Driver 내부 데이터의 동적 메모리 할당

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

주요 API

```c
kmalloc();
kfree();
```

---

# 9. filp->private_data

열린 파일별 Driver 데이터 저장 공간

```text
open()
   │
   ▼
kmalloc()
   │
   ▼
pkeyData
   │
   ├──────────────┐
   ▼              ▼
filp->private_data   request_irq()
   │                  │
   ▼                  ▼
read() / write()      ISR
```

`open()`에서 저장한 데이터를 `read()`, `write()`, `release()`에서 재사용

---

# 10. Wait Queue

Event 발생 전까지 Process를 Sleep 상태로 대기시키는 구조

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

주요 API

```c
DECLARE_WAIT_QUEUE_HEAD();
wait_event_interruptible();
wake_up_interruptible();
```

---

# 11. Blocking / Non-blocking I/O

`open()` Flag에 따른 `read()` 동작 구분

## Blocking I/O

데이터가 없으면 Event 발생까지 대기

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

데이터가 없으면 즉시 반환

```c
if (filp->f_flags & O_NONBLOCK)
    return -EAGAIN;
```

---

# 12. poll()

File Descriptor의 Event 발생 여부 확인

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

Driver

```c
poll_wait(filp, &wait_queue, wait);

if (event)
    mask |= POLLIN | POLLRDNORM;
```

Application

```c
poll(&pfd, 1, -1);
```

Busy Polling과 달리 Event 발생 전까지 Process 대기

---

# 13. /proc Interface

Kernel 내부 상태를 User Space에서 파일 형태로 확인하는 Virtual File System

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

실제 Storage에 저장되지 않는 가상 파일

예시

```bash
cat /proc/cpuinfo
cat /proc/meminfo
cat /proc/interrupts
cat /proc/modules
```

---

# 🔄 Final Event Flow

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
