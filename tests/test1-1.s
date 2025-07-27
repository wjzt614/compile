define i32 @main()
{
	declare i32 %l0
	entry
	%l0 = 0
	%l0 = 3
	br .L1
.L1:
	exit %l0
}
