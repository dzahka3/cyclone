def launch_cmds_startup():
    print("Configuring for rocksdb application")


def launch_cmds_server_gen(f, q, r, m, quorums, replicas, clients, ports):
    passwd=''
    if os.environ.has_key('CYCLONE_PASS'):
        passwd=os.environ.get('CYCLONE_PASS')
    cmd= ' echo ' + passwd + ' | sudo -S '
    cmd=cmd + 'rm -rf /mnt/pmem0/rocksdata\n'
    f.write(cmd)
    cmd=' echo ' + passwd + ' | sudo -S '
    cmd= cmd + 'rm -f /mnt/pmem0/rockswal/*\n'
    f.write(cmd)
    cmd=' echo ' + passwd + ' | sudo -S '
    cmd=cmd + ' LD_LIBRARY_PATH=/usr/lib:/usr/local/lib '
    cmd=cmd + '/home/cyclone/cyclone/cyclone.git/benchmarks/kvbench/novelsm/novelsm_loader\n'
    f.write(cmd)
    cmd=''
    if os.environ.has_key('RBT_SLEEP_USEC'):
        cmd=cmd + 'RBT_SLEEP_USEC=' + os.environ.get('RBT_SLEEP_USEC') + ' '
    cmd=cmd + ' echo ' + passwd + ' | sudo -S '
    cmd=cmd + ' PMEM_IS_PMEM_FORCE=1 '
    cmd=cmd + ' LD_LIBRARY_PATH=/usr/lib:/usr/local/lib '
    cmd=cmd + '/home/cyclone/cyclone/cyclone.git/benchmarks/kvbench/novelsm/novelsm_server '
    cmd=cmd + str(r) + ' '
    cmd=cmd + str(m) + ' '
    cmd=cmd + str(clients) + ' '
    cmd=cmd + 'config_cluster.ini config_quorum.ini ' +str(ports) + ' &> server_log &\n'
    f.write(cmd)

def launch_cmds_preload_gen(f, m, c, quorums, replicas, clients, machines, ports):
    cmd=''


def launch_cmds_client_gen(f, m, c, quorums, replicas, clients, machines, ports, bufsize):
    passwd=''
    if m >= replicas:
        client_machines=machines-replicas
        if client_machines > clients:
            client_machines = clients
        clients_per_machine=clients/client_machines
        c_start = clients_per_machine*(m - replicas)
        c_stop  = c_start + clients_per_machine
        if m == replicas + client_machines - 1:
            c_stop = clients
        if c == 0 and m < replicas + client_machines:
            cmd=''
            if os.environ.has_key('KV_FRAC_READ'):
                cmd=cmd + 'KV_FRAC_READ=' + os.environ.get('KV_FRAC_READ') + ' '
            if os.environ.has_key('KV_KEYS'):
                cmd=cmd + 'KV_KEYS=' + os.environ.get('KV_KEYS') + ' '    
            if os.environ.has_key('ACTIVE'):
                cmd=cmd + 'ACTIVE=' + os.environ.get('ACTIVE') + ' '    
            if os.environ.has_key('CYCLONE_PASS'):
                 passwd=os.environ.get('CYCLONE_PASS')
            cmd=cmd + ' echo ' + passwd + ' | sudo -S '
            cmd=cmd + ' LD_LIBRARY_PATH=/usr/lib:/usr/local/lib '
            #cmd=cmd + '/home/cyclone/cyclone/cyclone.git/test/rocksdb_client '
            cmd=cmd + '/home/cyclone/cyclone/cyclone.git/benchmarks/kvbench/novelsm/novelsm_async_client '
            cmd=cmd + str(c_start) + ' '
            cmd=cmd + str(c_stop) + ' '
            cmd=cmd + str(m) + ' '
            cmd=cmd + str(replicas) + ' '
            cmd=cmd + str(clients) + ' '
            cmd=cmd + str(quorums) + ' '
            #cmd=cmd + 'config_cluster.ini config_quorum ' + str(ports) + ' ' + ' &> client_log' + str(0) + ' &\n'
            cmd=cmd + 'config_cluster.ini config_quorum ' + str(ports) + ' ' + str(bufsize) + ' &> client_log' + str(0) + ' &\n'
            f.write(cmd)
        
def killall_cmds_gen(f):
    passwd=''
    if os.environ.has_key('CYCLONE_PASS'):
        passwd=os.environ.get('CYCLONE_PASS')
    f.write('echo ' + passwd + ' | sudo -S pkill novelsm_server\n')
    f.write('echo ' + passwd + ' | sudo -S pkill rocksdb_client\n')
    f.write('echo ' + passwd + ' | sudo -S pkill novelsm_async\n')
