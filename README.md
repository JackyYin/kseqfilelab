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

## Step 3. Insert some number into file

```shell=
echo 1 > /proc/seqfilelab/sample
echo 2 > /proc/seqfilelab/sample
echo 3 > /proc/seqfilelab/sample
```

---

## Step 4. Read list from file

```shell=
cat /proc/seqfilelab/sample
```
