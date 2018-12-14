

    Name(PR00, Package(20)
    {
        Package(4) {0x0003FFFF, Zero, LNKH, Zero}, 
        Package(4) {0x0003FFFF, 0x02, LNKH, Zero}, 
        Package(4) {0x0004FFFF, Zero, LNKH, Zero},          
        Package(4) {0x0005FFFF, Zero, LNKH, Zero}, 
        Package(4) {0x0005FFFF, 0x01, LNKH, Zero},        
        Package(4) {0x0001FFFF, Zero, LNKH, Zero},
        Package(4) {0x0001FFFF, One,  LNKH, Zero},        
        Package(4) {0x0008FFFF, Zero, LNKH, Zero}, 
        Package(4) {0x0009FFFF, Zero, LNKH, Zero},          
        Package(4) {0x000AFFFF, Zero, LNKA, Zero}, 
        Package(4) {0x000AFFFF, One,  LNKB, Zero}, 
        Package(4) {0x000BFFFF, Zero, LNKA, Zero}, 
        Package(4) {0x000EFFFF, Zero, LNKA, Zero}, 
        Package(4) {0x000EFFFF, 0x03, LNKD, Zero},         
        Package(4) {0x000FFFFF, Zero, LNKA, Zero},        
        Package(4) {0x0010FFFF, Zero, LNKA, Zero}, 
        Package(4) {0x0010FFFF, One,  LNKB, Zero},        
        Package(4) {0x0010FFFF, 0x3,  LNKD, Zero},   
        Package(4) {0x0012FFFF, Zero, LNKA, Zero},        
        Package(4) {0x0014FFFF, Zero, LNKA, Zero}        
    })
        
    Name(AR00, Package(20)
    {
        Package(4) {0x0003FFFF, Zero, Zero, 0x1F},         
        Package(4) {0x0003FFFF, 0x02, Zero, 0x27},         
        Package(4) {0x0004FFFF, Zero, Zero, 0x1B},        
        Package(4) {0x0005FFFF, Zero, Zero, 0x18}, 
        Package(4) {0x0005FFFF, 0x01, Zero, 0x1C},         
        Package(4) {0x0001FFFF, Zero, Zero, 0x29},
        Package(4) {0x0001FFFF, One,  Zero, 0x2A},         
        Package(4) {0x0008FFFF, Zero, Zero, 0x28}, 
        Package(4) {0x0009FFFF, Zero, Zero, 0x24},            
        Package(4) {0x000AFFFF, Zero, Zero, 0x10}, 
        Package(4) {0x000AFFFF, One,  Zero, 0x11}, 
        Package(4) {0x000BFFFF, Zero, Zero, 0x15}, 
        Package(4) {0x000EFFFF, Zero, Zero, 0x17}, 
        Package(4) {0x000EFFFF, 0x03, Zero, 0x14}, 
        Package(4) {0x000FFFFF, Zero, Zero, 0x15},
        Package(4) {0x0010FFFF, Zero, Zero, 0x14}, 
        Package(4) {0x0010FFFF, One,  Zero, 0x16},        
        Package(4) {0x0010FFFF, 0x3,  Zero, 0x15}, 
        Package(4) {0x0012FFFF, Zero, Zero, 0x14},        
        Package(4) {0x0014FFFF, Zero, Zero, 0x11}  
    })

    Name(PR01, Package(4)
    {
        Package(4) {0xFFFF, Zero, LNKH, Zero}, 
        Package(4) {0xFFFF, One, LNKH, Zero}, 
        Package(4) {0xFFFF, 0x02, LNKH, Zero}, 
        Package(4) {0xFFFF, 0x03, LNKH, Zero}
    })

    Alias (PR01, PR03)
    Alias (PR01, PR05)
    Alias (PR01, PR07)
    Alias (PR01, PR08)
      
    
    //PE0S APIC Routing
    Name(AR01, Package(4)
    {
        Package(4) {0xFFFF, Zero, Zero, 0x1C}, 
        Package(4) {0xFFFF, One, Zero, 0x1D}, 
        Package(4) {0xFFFF, 0x02, Zero, 0x1E}, 
        Package(4) {0xFFFF, 0x03, Zero, 0x1F}
    })    

    //PE2S APIC Routing
    Name(AR03, Package(4)
    {
        Package(4) {0xFFFF, Zero, Zero, 0x24}, 
        Package(4) {0xFFFF, One,  Zero, 0x25}, 
        Package(4) {0xFFFF, 0x02, Zero, 0x26}, 
        Package(4) {0xFFFF, 0x03, Zero, 0x27}
    })        


    //PE4S APIC Routing
    Name(AR05, Package(4)
    {
        Package(4) {0xFFFF, Zero, Zero, 0x18}, 
        Package(4) {0xFFFF, One, Zero, 0x19}, 
        Package(4) {0xFFFF, 0x02, Zero, 0x1A}, 
        Package(4) {0xFFFF, 0x03, Zero, 0x1B}
    })  
    

    //PE6S APIC Routing
    Name(AR07, Package(4)
    {
        Package(4) {0xFFFF, Zero, Zero, 0x19}, 
        Package(4) {0xFFFF, One, Zero, 0x1A}, 
        Package(4) {0xFFFF, 0x02, Zero, 0x1B}, 
        Package(4) {0xFFFF, 0x03, Zero, 0x18}
    })

    //PE7S APIC Routing
    Name(AR08, Package(4)
    {
        Package(4) {0xFFFF, Zero, Zero, 0x1D}, 
        Package(4) {0xFFFF, One, Zero, 0x1E}, 
        Package(4) {0xFFFF, 0x02, Zero, 0x1F}, 
        Package(4) {0xFFFF, 0x03, Zero, 0x1C}
    })

	      

    Name (PRSA, ResourceTemplate()
    {
        IRQ (Level, ActiveLow, Shared,) {3,4,5,6,7,10,11,12,14,15}
    })

    Alias (PRSA, PRSB)
    Alias (PRSA, PRSC)
    Alias (PRSA, PRSD)
    Alias (PRSA, PRSE)
    Alias (PRSA, PRSF)
    Alias (PRSA, PRSG)
    Alias (PRSA, PRSH)
    
