# Pasword Generation

The entropy harvester for the password generator is based on a scheme
documented at https://gist.github.com/endolith/2568571 .

When you plug the Password Thing into a USB port it starts to harvest
entropy by periodically sampling the lower buts of a fast-running timer. 
The bits are scrambled and combined to generate one bit of entropy for each
software cycle or SysTick. 

The default systick interval is 100ms, but when the amount of available
entropy is low, such as right after startup, the entropy harvester is run
every 1ms during idle time.  

Up to 256 bytes of entropy are stored in a queue to be used by the password
generator.  

The stored entropy is used to create password characters from the available
character set, which may be modified by the user through the configuration
interface.  

Some entropy is wasted on thrown-away bits to avoid modulo bias  so that the
randomness of the generated passwords are not dependent on the selected
character set.  

# Configuration interface issues
The configuration user interface is not very fault tolerant and bad input can
cause it to go into a state where nothing seems to work properly,  

In that case, just unplug and restart the configuration session.  If password
generation or entry stops working for a slot, use the Generate button to
generate a password for the slot in normal mode, and try again. 

