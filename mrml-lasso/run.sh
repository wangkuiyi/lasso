  #!/bin/bash
 
 
  #mpd --listenport=55555 &
  #cd PROCALL	  
  #./proc_cmd_run.sh < tlist.log "mv /home/relmlr/leostarzhou/data/output/output-*-of-00005 /home/relmlr/leostarzhou/data/input"
  #cd ..
  #exit
  
  #scp input-00000-of-00005 relmlr@172.26.3.109#36000:/home/relmlr/leostarzhou/data/input 
  #scp input-00001-of-00005 relmlr@172.26.3.107#36000:/home/relmlr/leostarzhou/data/input 
  #scp input-00002-of-00005 relmlr@172.26.3.108#36000:/home/relmlr/leostarzhou/data/input 
  #scp input-00003-of-00005 relmlr@172.26.3.110#36000:/home/relmlr/leostarzhou/data/input 
  #scp input-00004-of-00005 relmlr@172.26.3.111#36000:/home/relmlr/leostarzhou/data/input 
  #exit

  rm /home/relmlr/leostarzhou/data/base_dir/*
  cd PROCALL	  
  ./proc_cmd_run.sh < tlist.log "rm /home/relmlr/leostarzhou/data/base_dir/*"	
  cd ..
  date_start=$(date +%s)

  rm flag_file  
  [[ -e  flag_file ]] &&{
	rm flag_file	
  } || {
   
  while [ ! -e flag_file ]
  do
  
  file=`ls -th /home/relmlr/leostarzhou/data/base_dir/ | head -1 | awk '{print $NF}'` 
  echo $file
  ip_set="172.26.3.110 172.26.3.111 172.26.3.108 172.26.3.107"	
  for ip in $ip_set
  do
	scp /home/relmlr/leostarzhou/data/base_dir/${file} relmlr@${ip}#36000:/home/relmlr/leostarzhou/data/base_dir	
  done
  
  mpiexec -machinefile ./machine-list -np 6 mrml_mappers_and_reducers	\
  --mrml_num_map_workers=5					\
  --mrml_num_reduce_workers=1				\
  --mrml_input_format=text					\
  --mrml_output_format=recordio					\
  --mrml_input_filebase=/home/relmlr/leostarzhou/data/input/input		\
  --mrml_output_filebase=/home/relmlr/leostarzhou/data/output/output		\
  --mrml_mapper_class=ComputeGradientMapper_dense			\
  --mrml_reducer_class=UpdateModelReducer_dense			\
  --mrml_log_filebase=/home/relmlr/leostarzhou/data/lr-log  \
  --states_file_dir=/home/relmlr/leostarzhou/data/base_dir      \
  --flag_file=flag_file          \
  --states_filebase=states_filebase   \
  --max_fea_num=100001 
  done
  } 
  
  date_end=$(date +%s)
  echo "using time :"
  echo $(($date_end-$date_start))
