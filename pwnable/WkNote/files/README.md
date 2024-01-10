# WkNote

## Setup

1. install VirtualBox  
2. install Vagrant  
    - https://developer.hashicorp.com/vagrant/downloads  
    - The version of vagrant installed from the default package repository is outdated and may not work properly.  
3. run `vagrant up`  
4. automatic driver installation on VM  
    - After rebooting the VM, a dialog box will appear asking for permission to install driver.  

## Rule

You need to execute your exploit on VM on a machine located in the front booth and show it to the organizer.  
You can transfer the exploit to the target VM using FTP.  

You are not allowed to operate outside the VM.  
If the VM needs to be restarted due to a freeze or other situation, we will take care of this.  

There is no limit to the number of attempts, unless other teams are waiting their turn.  
Even if there is, you can run your exploit repeatedly for up to 5 minutes in one attempt.  

You can take this challenge until 16:00 on the last day.  

### Tasks

Launch `cmd.exe` with system privilege and execute the commands below.  

1. whoami  
    - Make sure you have system privilege.  
2. echo [team name] > C:\hacked.txt  
3. echo @[discord id] >> C:\hacked.txt  
    - We will check your team/discord ID and give you the flag via DM later.  

