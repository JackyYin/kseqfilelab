# seq_file lab

A simple lab inspired by [doc](https://www.kernel.org/doc/Documentation/filesystems/seq_file.txt)

> The
> sequence will continue until the user loses patience and finds something
> better to do.

## Step 1. Build Kernel Module

In project root directory:

```shell=
make
```
---

## Step 2. Install Kernel Module

In project root directory:

```shell=
sudo insmod kmod/seqfile.ko
```

---

## Step 3. Here we go

```shell=
cat /proc/seqsample
```
