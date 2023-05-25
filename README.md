# Introduction
**PlotXY** is a free piece of software able to make line plots and bar charts from data deriving from measuring devices or simulation programs. 

PlotXY was created during 1998, continuously maintained and upgraded up to current year by Massimo Ceraolo from the University of Pisa.
It initial motivation was to supply a powerful and easy-to-use plotting program for outputs from the freeware ATP program.

In recent years, it has completely been re-written, using the great power of Qt simulation environment. Modern features have been added to keep it up-to-date, such as the capability to output plots in SVG PNG and PDF and to manage modern 4k resolutions and multiple screens.
# Screenshot
![PlotXY](https://github.com/DonCN/PlotXY_OpenSource/assets/4151162/a75c38d1-1f27-4985-9a9c-462df44ac35d)

# Features and capabilities
- **Plotting**. It is able to plot signals both versus time or versus other signals, to plot several variables simultaneously taken from the same file or different files, fast multi-layer zoom and unzoom, peeking at numerical values. It can also show bar charts, which are very useful to display the output of Fourier analysis.
- **Input/output**.  It is able to read some exoteric formats such as ATP pl4 and COMTRADE , but also has some support for Mathworks’ Matlab, National Instruments’ LabView and other formats. Plots or charts can be copied to clipboard, or saved in PNG, SVG, PDF files. New files containing a selected number of variables among those present in the input files can be created.
-  **Post-process**. Formulas containing sums and/or products of any number of signals, mixing them with constants and using parenthesis are analyzed and plotted. Moreover the integral of input variables or combination of input variables can be created and plotted against time, thus allowing computing energies from powers, stored charges from currents, etc. 
- **Open-source**. It is open-source and therefore anyone can propose improvements or additions.
- **portable**. It is very small and portable, does not require the execution of an installation routine.

# Development and platform
**PlotXY** is developed using the Qt platform, which allows good GUI design, and already available for Windows, Macintosh, and Linux systems.

# License
This program is free software: you can redistribute it under the terms of GNU Public License version 3 as published by the Free Software Foundation.

PLOTXY AND ALL THE RELATED MATERIAL INCLUDED IN THE DISTRIBUTION PLOTXY.ZIP FILE OR AVAILABLE FROM GITHUB IS SUPPLIED "AS-IS" THE AUTHOR OFFERS NO WARRANTY OF ITS FITNESS FOR ANY PURPOSE WHATSOEVER, AND ACCEPTS NO LIABILITY WHATSOEVER FOR ANY LOSS OR DAMAGE INCURRED BY ITS USE.
