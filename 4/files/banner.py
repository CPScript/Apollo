import os
import platform
from setup import colors
from setup.colors import r,g,y,c , w
from colorama import Fore , Back

GB = Back.GREEN
YB = Back.YELLOW
WB = Back.RED



logo = f"""


                                       ...
                                    ;::::;
                                  ;::::; :;
                                ;:::::'   :;
                               ;:::::;     ;.
                             ,:::::'       ;           OOO
                             ::::::;       ;          OOOOO
                             ;:::::;       ;         OOOOOOOO
                            ,;::::::;     ;'         / OOOOOOO
                          ;:::::::::`. ,,,;.        /  / DOOOOOO
                        .';:::::::::::::::::;,     /  /     DOOOO
                       ,::::::;::::::;;;;::::;,   /  /        DOOO
 ▄█     ▄███████▄     ;`::::::`'::::::;;;::::: ,#/  /          DOOO                                        
███    ███    ███    :`:::::::`;::::::;;::: ;::#  /            DOOO                                          
███▌   ███    ███    ::`:::::::`;:::::::: ;::::# /              DOO                                        
███▌   ███    ███    `:`:::::::`;:::::: ;::::::#/               DOO                                       
███▌ ▀█████████▀     :::`:::::::`;; ;:::::::::##                OO                                       
███    ███           ::::`:::::::`;::::::::;:::#                OO                                        
███    ███           `:::::`::::::::::::;'`:;::#                O                                       
█▀    ▄████▀          `:::::`::::::::;' /  / `:#                                        
                       ::::::`:::::;'  /  /   `# 
██████╗ ███████╗ █████╗ ██████╗ ███████╗██████╗ 
██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔════╝██╔══██╗
██████╔╝█████╗  ███████║██████╔╝█████╗  ██████╔╝
██╔══██╗██╔══╝  ██╔══██║██╔═══╝ ██╔══╝  ██╔══██╗ {Back.RESET}
██║  ██║███████╗██║  ██║██║     ███████╗██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝     ╚══════╝╚═╝  ╚═╝
            {y}[{w}-{y}] {c}Author: {w}CPScript {r}|{g}    

{w}<{y}/{w}> {GB}{w}Github : CPScript {Back.RESET}
{w}<{y}/{w}> {GB}{w}About : IP Address info Grabber {Back.RESET}  
{w}<{y}/{w}> {w}Made useing Python {Back.RESET}  
                           ▄▄▀█▄   ▄       ▄     
                           ▀▀▀██  ███     ███    
                            ▄██▀ █████   █████   
                          ███▀▄███ ███ ███ ███ ▄
                          ▀█████▀   ▀███▀   ▀██▀



"""
c = colors
try:
    from colorama import Fore, Style
except ModuleNotFoundError:
    os.system("pip install colorama")

def banner():
    print(c.ran + logo)



def banner2():


    print(c.ran,"\n" + "|"+ Style.BRIGHT + Fore.LIGHTCYAN_EX, "- " * 4, "  [+] About CPScript ", "- " * 4 + c.ran + "|")
    print(c.ran, "\n" + "|"+ Style.BRIGHT + Fore.LIGHTYELLOW_EX, "- " * 4, " [+] CPS is a coder that works on Networking, Malware, and Hacks ", "- " * 4+c.ran + "|")
    print(c.ran , "\n"+ "|"+ Style.BRIGHT + Fore.LIGHTRED_EX, "- " * 4, "[+] You can find more here--> https://github.com/CPScript/", "- " * 3+c.ran + "|")
    print(c.ran + "\n"+ "|" + "*" * 60+c.ran + "|")

def clear():
    s = platform.platform()
    if "Windows" in s:
        os.system("cls")
    else:
        os.system("cls") if "Windows" in platform.platform() else os.system("clear")
