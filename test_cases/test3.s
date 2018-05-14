# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

		.text
_start:
		addiu	$t0,$0,80
		addi	$t1,$0,0
		lui	$s0,0x0040
		addiu	$s0,$s0,0x1000
loop:		subi	$t0,$t0,1		
		sw	$t1,0($s0)
		addi	$t1,$t1,4
		addi	$s0,$s0,4
		bne	$t0,$0,loop
done:		addi	$a0,$t1,4	
