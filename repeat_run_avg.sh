#!/bin/bash

#workload, iterations, line 44 output

# set experimental values
workloads=(0.05 0.1 0.15 0.2 0.25 0.3 0.35 0.4 0.45 0.5 0.55 0.6 0.65 0.7 0.75 0.8 0.85 0.9 0.95 1.0 1.05 1.1 1.15 1.2 1.25 1.3 1.35 1.4 1.45 1.5)
networkUp=30
networkDown=120
seed=0
iterations=10

output_dir="./tmp"

# set the name of the result_file
result_file="$output_dir/result_${networkUp}_${networkDown}_${iterations}.txt"

echo "Workload Section Power Util CPU_Power Memory_Power Network_Power Offloading_Ratio CPU_Frequency_1 CPU_Frequency_0.564 CPU_Frequency_0.327 CPU_Frequency_0.25 CPU_Frequency_0.182 Period_Violation Cost Cpu_Memory_Cost Network_Cost Baseline_Cost HW_Cost Rental_Cost" > $result_file

# conduct experiments according to workload
for workload in "${workloads[@]}"; do
    echo "Running workload: $workload"
    # set utilCpu
    utilCpu=$(echo "$workload - 0.025" | bc)

    # Initializing variables for calculating averages
    declare -A sums
    declare -A counts

    # sections=("Tovs" "DVFS")
    sections=("Tovs")
    metrics=("Power" "Util" "CPU_Power" "Memory_Power" "Network_Power" "Offloading_Ratio" "CPU_Frequency_1" "CPU_Frequency_0.564" "CPU_Frequency_0.327" "CPU_Frequency_0.25" "CPU_Frequency_0.182" "Period_Violation" "Cost" "Cpu_Memory_Cost" "Network_Cost" "Baseline_Cost" "HW_Cost" "Rental_Cost")

    for section in "${sections[@]}"; do
        for metric in "${metrics[@]}"; do
            sums["$section $metric"]=0
        done
        counts["$section"]=0
    done

    for ((i = 1; i <= iterations; i++)); do
        echo "Iteration $i for workload $workload..."

        # run "run.sh"
        output=$(./run_extend.sh $workload $utilCpu $networkUp $networkDown $seed)

        # Extract values by section
        for section in "${sections[@]}"; do
            case $section in
                "Tovs")
                    power=$(echo "scale=6; $output" | sed -n '3p' | awk '{print $2}')
                    util=$(echo "scale=6; $output" | sed -n '3p' | awk '{print $4}')
                    cpu_power=$(echo "scale=6; $output" | sed -n '4p' | awk '{print $3}')
                    memory_power=$(echo "scale=6; $output" | sed -n '4p' | awk '{print $6}')
                    network_power=$(echo "scale=6; $output" | sed -n '4p' | awk '{print $9}')
                    ratio=$(echo "scale=6; $output" | sed -n '5p' | awk '{print $3}')
                    freq_1=$(echo "$output" | sed -n '8p' | awk '{print $1}')
                    freq_0_564=$(echo "$output" | sed -n '8p' | awk '{print $2}')
                    freq_0_327=$(echo "$output" | sed -n '8p' | awk '{print $3}')
                    freq_0_25=$(echo "$output" | sed -n '8p' | awk '{print $4}')
		    freq_0_182=$(echo "$output" | sed -n '8p' | awk '{print $5}')
		    period_violation=$(echo "$output" | sed -n '9p' | awk '{print $3}')
		    cost=$(echo "$output" | sed -n '10p' | awk '{print $3}')
		    cpu_memory_cost=$(echo "$output" | sed -n '11p' | awk '{print $4}')
		    network_cost=$(echo "$output" | sed -n '12p' | awk '{print $4}')
		    base_cost=$(echo "$output" | sed -n '13p' | awk '{print $4}')
		    hw_cost=$(echo "$output" | sed -n '14p' | awk '{print $4}')
		    rental_cost=$(echo "$output" | sed -n '15p' | awk '{print $4}')
		    ;;
                # "DVFS")
                    # power=$(echo "scale=6; $output" | sed -n '11p' | awk '{print $2}')
                    # util=$(echo "scale=6; $output" | sed -n '11p' | awk '{print $4}')
                    # cpu_power=$(echo "scale=6; $output" | sed -n '12p' | awk '{print $3}')
                    # memory_power=$(echo "scale=6; $output" | sed -n '12p' | awk '{print $6}')
                    # network_power=$(echo "scale=6; $output" | sed -n '12p' | awk '{print $9}')
                    # ratio=$(echo "scale=6; $output" | sed -n '13p' | awk '{print $3}')
                    # freq_1=$(echo "$output" | sed -n '16p' | awk '{print $1}')
                    # freq_0_5=$(echo "$output" | sed -n '16p' | awk '{print $2}')
                    # freq_0_25=$(echo "$output" | sed -n '16p' | awk '{print $3}')
                    # freq_0_125=$(echo "$output" | sed -n '16p' | awk '{print $4}')
		    # period_violation=$(echo "$output" | sed -n '17p' | awk '{print $3}')
                    # ;;
            esac

            # calculate sum of values
            if [[ -n "$power" ]]; then
                sums["$section Power"]=$(echo "${sums["$section Power"]} + $power" | bc)
                sums["$section Util"]=$(echo "${sums["$section Util"]} + $util" | bc)
                sums["$section CPU_Power"]=$(echo "${sums["$section CPU_Power"]} + $cpu_power" | bc)
                sums["$section Memory_Power"]=$(echo "${sums["$section Memory_Power"]} + $memory_power" | bc)
                sums["$section Network_Power"]=$(echo "${sums["$section Network_Power"]} + $network_power" | bc)
                sums["$section Offloading_Ratio"]=$(echo "${sums["$section Offloading_Ratio"]} + $ratio" | bc)
                sums["$section CPU_Frequency_1"]=$(echo "${sums["$section CPU_Frequency_1"]} + $freq_1" | bc)
                sums["$section CPU_Frequency_0.564"]=$(echo "${sums["$section CPU_Frequency_0.564"]} + $freq_0_564" | bc)
                sums["$section CPU_Frequency_0.327"]=$(echo "${sums["$section CPU_Frequency_0.327"]} + $freq_0_327" | bc)
                sums["$section CPU_Frequency_0.25"]=$(echo "${sums["$section CPU_Frequency_0.25"]} + $freq_0_25" | bc)
		sums["$section CPU_Frequency_0.182"]=$(echo "${sums["$section CPU_Frequency_0.182"]} + $freq_0_182" | bc)
		sums["$section Period_Violation"]=$(echo "${sums["$section Period_Violation"]} + $period_violation" | bc)
		sums["$section Cost"]=$(echo "${sums["$section Cost"]} + $cost" | bc)
		sums["$section Cpu_Memory_Cost"]=$(echo "${sums["$section Cpu_Memory_Cost"]} + $cpu_memory_cost" | bc)
		sums["$section Network_Cost"]=$(echo "${sums["$section Network_Cost"]} + $network_cost" | bc)
		sums["$section Baseline_Cost"]=$(echo "${sums["$section Baseline_Cost"]} + $base_cost" | bc)
		sums["$section HW_Cost"]=$(echo "${sums["$section HW_Cost"]} + $hw_cost" | bc)
		sums["$section Rental_Cost"]=$(echo "${sums["$section Rental_Cost"]} + $rental_cost" | bc)
                counts["$section"]=$((counts["$section"] + 1))
            fi
        done
    done

    # calculate average and save results
    for section in "${sections[@]}"; do
        if [[ ${counts["$section"]} -gt 0 ]]; then
            echo "$workload $section $(for metric in "${metrics[@]}"; do echo -n "$(echo "scale=6; ${sums["$section $metric"]} / ${counts["$section"]}" | bc) "; done)" >> $result_file
        fi
    done
done
