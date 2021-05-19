
FilePath = "~/Documents/Codjo/Git/TeraRanger-Evo/macOs/";

Tfile = fopen(FilePath + 'Toutput.bin','r','ieee-le');
Dfile = fopen(FilePath + 'Doutput.bin','r','ieee-le');

NUm = 10000; % Number of points. Must match the file
time = [];
data = [];

for i = 1:NUm-1
    temp = fread(Tfile,1,'uint32');
    time(i) = temp;
    data(i) = fread(Dfile,1,'uint16');
end

fclose(Tfile);
fclose(Dfile);

data = movmean(data,50);


figure; hold on;
plot(time,data);
xlim([0 10000000]);
% plot(time,data2);

%%
timediff = sum(diff(time))/(length(time)-1)/10^6;    % Sample Rate
Fs = 1/(timediff);                                % Sample Freq
% Fs = 115200;          % Sampling frequency
% T = 1/Fs;             % Sampling period
L = length(data);        % Length of signal

 
Y = fft(data);

P2 = abs(Y/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);


f = Fs*(0:(L/2))/L;

figure;
plot(f(2:end),P1(2:end));
xlim([0 1]);
title('Single-Sided Amplitude Spectrum of X(t)');
xlabel('f (Hz)');
ylabel('|P1(f)|');

[M,I] = max(P1(2:end));
display(f(I+1));


%%

fmix=40;
carrier=timediff*(0:L-1);
carrier=cos(carrier);

dataHPF=data;
dataHPF(1)=0;

datamixed=carrier.*dataHPF;

Ymix=fft(datamixed);
plot(f,Ymix);