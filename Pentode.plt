mu=10.1613
kg1=0.80634
ex=1.42701
kp=46.9059
kvb=0.00277402
kvb1=6.32287
vct=0.788021
kg2=10.6521
a=0.000503609
alpha=0.117586
beta=0.0348923
gamm=1.87059
tau=0.136979
rho=0.0308172
theta=2.18828
psi=7.11949

f(v)=sqrt(kvb+v*kvb1+v*v)
lc(v1,v2)=1/mu+(v1+vct)/sqrt(kvb+kvb1*v2+v2*v2)
epk(v1,v2)=(v2/kp*log(1+exp(kp*lc(v1,v2))))**ex

shift(v1)=beta*(1-alpha*v1)
g(v1,v3)=1/exp((shift(v1)*v3)**gamm)
ia(v1,v2,v3)=epk(v1,v2)*((1/kg1-1/kg2)*(1-g(v1,v3)) + a*v3/kg1)

set yrange [0:150]
set xrange [0:300]

plot ia(0,285,x),ia(-2,285,x),ia(-10,285,x)