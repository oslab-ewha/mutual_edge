function usage() {
    cat <<EOF
Usage: run_batch.sh <util> <util cpu> <network_up> <network_down> <seed>
EOF
}

if [ $# -lt 3 ]; then
    usage
    exit 1
fi

GASTASK=`./gastask`

GASGEN=`./gasgen`

utilTarget=$1
utilCpu=$2
networkUp=$3
networkDown=$4
seed=$5

gastask_conf=./tmp/gastask_$utilTarget+$$.conf


COMMON_CONF="\
# max_generations n_populations cutoff penalty
*genetic
10000 100 3.0 1.5

# wcet_min wcet_max mem_total util_cpu util_target n_tasks task_size_min task_size_max input_size_min input_size_max output_size_min output_size_max
*gentask
500 1000 45000000 $utilCpu $utilTarget 100 500 750 100 500 100 250

# uplink_min uplink_max downlink_min downlink_max n_networks
*gennetwork
30 30 120 120 100

# intercept_out_min intercept_out_max intercept_in_min intercept_in_max n_net_commanders
*gennetcommander
1 5 5 7 100

# wcet_scale power_active power_idle
*cpufreq
1    100    1
0.564  31.8096 0.318096
0.327 10.6929 0.106929
0.25 6.25 0.0625
0.182 3.3124 0.03124

# type max_capacity wcet_scale power_active power_idle
*mem
dram  45000000 1    0.01   0.001

# type computation_power power_active power_idle max_capacity offloading_limit
*cloud
mec  10   60   3.5   45000000   1.0

# offloading_ratio 
*offloadingratio
0

# uplink_data_rate downlink_data_rate
*network"

echo "$COMMON_CONF" <<EOF >$gastask_conf
EOF

'./gasgen' $gastask_conf
cat ./network_generated.txt >>$gastask_conf

echo "
# intercept_out intercept_in
*netcommander" >>$gastask_conf

cat ./network_commander_generated.txt >>$gastask_conf

echo "
# wcet period memreq mem_active_ratio input_data_size output_data_size
*task" >>$gastask_conf

cat ./task_generated.txt >>$gastask_conf

mkdir ./tmp/output_$utilTarget+$$
OUTPUT=./tmp/output_$utilTarget+$$
mv ./task_generated.txt $OUTPUT/gen_task_generated_$utilTarget+$$.txt
mv ./network_commander_generated.txt $OUTPUT/gen_network_commander_generated_$utilTarget+$$.txt

touch $OUTPUT/output_$utilTarget+$networkUp.txt
echo "*tovs\n" >> $OUTPUT/output_$utilTarget+$networkUp.txt
./gastask -s $seed $gastask_conf | tee -a $OUTPUT/output_$utilTarget+$networkUp.txt
mv task.txt $OUTPUT/task_$utilTarget+$networkUp+tovs.txt
# sed -i '36s/1/#1/' $gastask_conf

# echo "\n*dvfs\n" >> $OUTPUT/output_$utilTarget+$networkUp.txt
# ./gastask -s $seed $gastask_conf | tee -a $OUTPUT/output_$utilTarget+$networkUp.txt
# mv task.txt $OUTPUT/task_$utilTarget+$networkUp+dvfs.txt
# sed -i '36s/#1/1/' $gastask_conf

mv $gastask_conf $OUTPUT/gastask_$utilTarget+$$.conf
