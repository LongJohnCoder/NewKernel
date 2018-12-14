//
//  Step to update bios in Ubuntu 14.04-64bit
//
Step1: Copy the files ¡°flash¡± & ¡°ufudev.ko¡± to the home directory;

Step2: Open the terminal and switch to root permissions; 

Step3: Install driver: insmod ufudev.ko
       Set up node   : mknod /dev/ufudev.ko c 204 0

Step4: Run the flash program: ./flash -bfu xx.bin;

Note: The flash tool was build for Ubuntu14.04_amd64, if update bios in any other Linux OS,the flash tool 
      need to be rebuild!