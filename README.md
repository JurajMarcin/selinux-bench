# SELinux Benchmarks

## `file_create`

Runs `security_compute_create_name()` for each filename transition data in the input file.

Input file can be generated from `policy.conf` by `compute_trans_data.py` script.

Example:

```sh
$ make file_create
$ checkpolicy -b -F -M -o policy.conf /etc/selinux/targeted/policy/policy.33
$ python compute_trans_data.py < policy.conf > bench_data.txt
$ ./file_create bench_data.txt
```
