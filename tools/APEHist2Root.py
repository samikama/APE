import os,time,re,string,sys,shutil
import matplotlib 
matplotlib.use('Agg')
from matplotlib import pyplot as plt
import numpy as np
import datetime
from matplotlib import cm as cm
import pkgutil

def parseAPEStats(logFile):
    if not os.path.exists(logFile):
        return None
    maxQre=re.compile(r"^(APE <.*>:? *)?Max queue was ([0-9]+) ite.*")
    compRe=re.compile(r"^(APE <.*>:? *)?Completed ([0-9]+) reques.*")
    thrRe=re.compile(r"^(APE <.*>:? *)?(Runner|Sender) thread ([0-9]+)? ?was running \D+ ([0-9]+\.[0-9]+) seconds \D+([0-9]+\.[0-9]+)% of the t.*")
    modRe=re.compile(r"^(APE <.*>:? *)?--- Module (\d+) (.*)")
    avgRe=re.compile(r"^(APE <.*>:? *)?Avg time for completing a request is (\d+.\d+) .*")
    totData=re.compile(r"^(APE <.*>:? *)?Total data transferred (\d+) .* is (\d+) .*")
    logHdr=re.compile(r"^APE <.*>:? *(.*)")
    REs=[maxQre,compRe,thrRe,modRe,avgRe,totData]
    stats={"Worker":{},"Module":{},"ModIds":{}}
    ParseHists=False
    histLines=[]
    ModHists={}
    with open(logFile,"rb") as f:
        for line in f:
            if ParseHists:
                if "-------- -- -- -- -- -- --" in line:
                    ParseHists=False
                    ModHists[modId[1]]=histLines
                    histLines=[]
                    continue
                match=logHdr.match(line)
                if match:
                    #print match.group(1)
                    histLines.append(match.group(1).strip())
                else:
                    histLines.append(line.strip())
            else:
                for i,r in enumerate(REs):
                    match=r.match(line)
                    if match:
                        #print match.group,line
                        if i == 0: #max queue
                            stats["MaxQueue"]=match.group(2)
                        elif i == 1: # numrequests 
                            stats["NumRequests"]=match.group(2)
                        elif i == 2: # thread
                            if "Sender" in line:
                                stats["Sender"]=(match.group(4),match.group(5))
                            else:
                                stats["Worker"][match.group(3)]=(match.group(4),match.group(5))
                        elif i == 3: # module
                            modId=(match.group(2),match.group(3))
                            stats["ModIds"][modId[0]]=modId[1]
                            ParseHists=True
                        elif i == 4: #avg time
                            stats["AverageTime"]=match.group(2)
                        elif i == 5: #transferred data size
                            stats["TotalData"]=(match.group(2),match.group(3))
    for k,v in ModHists.items():
        stats["Module"][k]=v
    return stats

def Hists2Plots(stats):
    plt.ioff()
    print "Plot Generation"
    for m in stats["Module"]:
        print "Working on Module %s"%m
        hists=stats["Module"][m]
        nHists=len(hists)/4
        #print "Hists for Module",m,"=",hists,"Total ",nHists
        for h in xrange(nHists):
            if "|" and ";" in hists[h*4]:
                HistName,Labels=string.split(hists[h*4],"|")
                Labels="%s "%Labels
                #print Labels,[ x.strip() for x in string.split(Labels.strip()," ; ")]
                Title,XLabel,YLabel,ZLabel=[ x.strip() for x in string.split(Labels," ; ")]
            else:
                HistName=hists[h*4].strip()
                Title=HistName
                XLabel=""
                YLabel=""
            #print Title
            axisInfo=map(float,string.split(hists[h*4+1]))
            nBinsX,Xmin,Xmax=axisInfo[:3]
            Hist2D=False
            if len(axisInfo)>8:
                Hist2D=True
                nBinsY,Ymin,Ymax=axisInfo[3:6]
                #print axisInfo,axisInfo[6:]
                nEnt,sumW,sumW2,sumWx,sumWx2=axisInfo[6:]
            else:
                nEnt,sumW,sumW2,sumWx,sumWx2=axisInfo[3:]                
            binVals=np.asarray(map(float,string.split(hists[h*4+2])))
            xbins=np.linspace(Xmin,Xmax,nBinsX+1)
            if Hist2D:
                ybins=np.linspace(Ymin,Ymax,nBinsY+1)
                bincentY=ybins[:-1]+np.gradient(ybins)[0]/2.0                
            bincentX=xbins[:-1]+np.gradient(xbins)[0]/2.0
            fig,ax=plt.subplots()
            if Hist2D:
                #print "Nbins= ",nBinsX,nBinsY,len(binVals),HistName
                binVals=binVals.reshape(nBinsY+2,nBinsX+2)
                hstats=[binVals[0,0],#xy underflow
                        np.sum(binVals[0,1:-1]),#y underflow
                        binVals[0,-1],#x overflow,y underflow
                        np.sum(binVals[1:-1,0]),#x underflow, y in range
                        np.sum(binVals[1:-1,1:-1]), #entries in range 
                        np.sum(binVals[1:-1,-1]),#x overflow, y in range
                        binVals[-1,0], #x underflow y-overflow
                        np.sum(binVals[-1,1:-1]),# x in range, y-overflow
                        binVals[-1,-1] # xy overflow
                ]
                binVals=binVals[1:-1,1:-1]
            else:
                hstats=[binVals[0],binVals[-1]]
                binVals=binVals[1:-1]
            if not Hist2D:
                bins1=ax.bar(xbins[:-1],binVals,width=np.gradient(xbins)[0])
            else:
                #print (Xmin,Xmax,Ymin,Ymax)
                cmSami=cm.gist_rainbow
                cmSami='binary'
                im=ax.imshow(binVals,extent=(Xmin,Xmax,Ymin,Ymax),interpolation='nearest',
                           cmap=cmSami,origin='lower',aspect='auto')
                position=fig.add_axes([0.93,0.1,0.02,0.63])
                #fig.colorbar(im,shrink=0.8,pad=0.05,cax=position)
                fig.colorbar(im,cax=position)
            #integral=np.sum(bincentX*binVals)
            nEntriesIn=nEnt
            if nEntriesIn==0. : 
                nEntriesIn=1.0
            mean=sumW/nEntriesIn
            stdDev=np.sqrt(sumW2/nEntriesIn-(mean*mean))
            ax.set_ylabel(YLabel)
            ax.set_xlabel(XLabel)
            ax.set_title(Title)
            HistFileName="%s.png"%(string.replace(HistName," ","_"))
            props=dict(boxstyle='round', facecolor='wheat', alpha=0.5)
            if not Hist2D:
                plt.rc('text',usetex=False)
                textstr="$\mu=%.2f$\n$\sigma=%.2f$\n$\mathrm{uflow}=%d$\n$\mathrm{oflow}=%d$\n$\mathrm{Entries}=%d$"%(mean,stdDev,hstats[0],hstats[1],nEnt)
            else:
                plt.rc('text',usetex=True)
                textstr=r'\begin{tabular}{ccc}$\mu$&=&%.2f\\$\sigma$&=&%.2f\\Entries&=&%d\end{tabular}'%(mean,stdDev,nEnt)
                textstr=r'%s\\\begin{tabular}{|c|c|c|}\hline'%(textstr)
                textstr=r'%s %.2f & %.2f & %.2f\\\hline'%(textstr,hstats[6],hstats[7],hstats[8])
                textstr=r'%s %.2f & %.2f & %.2f\\\hline'%(textstr,hstats[3],hstats[4],hstats[5])
                textstr=r'%s %.2f & %.2f & %.2f\\\hline\end{tabular}'%(textstr,hstats[0],hstats[1],hstats[2])

            ax.text(0.80, 1.09, textstr, transform=ax.transAxes, fontsize=12,
                    verticalalignment='top', bbox=props)
            plt.savefig(HistFileName,bbox_inches='tight',pad_inches=0.08)
            plt.close(fig)

def Hists2Root(stats,fileName="APEHists.root"):
    rootLoader=pkgutil.find_loader('ROOT')
    if rootLoader is None:
        sys.exit("Can't import ROOT module. Did you source ROOT environment?")
    from ROOT import TFile, TH1F,TH2F
    outFile=TFile(fileName,"RECREATE")
    print "Root conversion to file %s"%fileName
    for m in stats["Module"]:
        hists=stats["Module"][m]
        
        dirName=string.replace(m.strip()," ","_")
        outFile.cd()
        newDir=outFile.mkdir(dirName)
        if newDir is not None:
            newDir.cd()
        print "Working on module %s"%m
        nHists=len(hists)/4
        for h in xrange(nHists):
            if "|" and ";" in hists[h*4]:
                HistName,Labels=string.split(hists[h*4],"|")
                Labels="%s "%Labels
                #print Labels,[ x.strip() for x in string.split(Labels," ; ")]
                Title,XLabel,YLabel,ZLabel=[ x.strip() for x in string.split(Labels," ; ")]
                HistName=string.replace(HistName," ","_")
            else:
                HistName=hists[h*4].strip()
                Title=HistName
                HistName=string.replace(HistName," ","_")
                XLabel=""
                YLabel=""
                ZLabel=""
            axisInfo=map(float,string.split(hists[h*4+1]))
            nBinsX,Xmin,Xmax=axisInfo[:3]
            nBinsX=int(nBinsX)
            Xmin=float(Xmin)
            Xmax=float(Xmax)
            Hist2D=False
            if len(axisInfo)>8:
                Hist2D=True
                nBinsY,Ymin,Ymax=axisInfo[3:6]
                nBinsY=int(nBinsY)
                Ymin=float(Ymin)
                Ymax=float(Ymax)
                nEnt,sumW,sumW2,sumWx,sumWx2=map(float,axisInfo[6:])
                histStats=np.array(axisInfo[6:],dtype=np.float)
            else:
                nEnt,sumW,sumW2,sumWx,sumWx2=map(float,axisInfo[3:])
                histStats=np.array(axisInfo[3:],dtype=np.float)
            binVals=np.asarray(map(float,string.split(hists[h*4+2])))
            if Hist2D:
                hist=TH2F(HistName,Title,nBinsX,Xmin,Xmax,nBinsY,Ymin,Ymax)
                hist.SetXTitle(XLabel)
                hist.SetYTitle(YLabel)
                hist.SetZTitle(ZLabel)
            else:
                hist=TH1F(HistName,Title,nBinsX,Xmin,Xmax)
                hist.SetXTitle(XLabel)
                hist.SetYTitle(YLabel)
            it=np.nditer(binVals,flags=['f_index'])
            while not it.finished:
                hist.SetBinContent(it.index,it[0])
                it.iternext()
            hist.PutStats(histStats)
            hist.Write()
        outFile.Save()

if "__main__" in __name__:
    import optparse as op
    p = op.OptionParser(usage = "%")
    p.add_option("-R", "--root-file", type=str, help="Name of the root file",dest="rootFile",default=None)
    p.add_option("-a", "--ape-log", type=str, help="Ape log file",dest="apelog")
    p.add_option("-P", "--png-only", action="store_true", help="Generate PNG files",dest="pngOnly",default=False)    
    opts,args=p.parse_args()
    if opts.apelog is None:
        sys.exit("ApeLog file is needed")
    if opts.rootFile is None and opts.pngOnly is False:
        sys.exit("Need to give at least either PNG or Root option")
    modStats=parseAPEStats(opts.apelog)
    if(opts.pngOnly):
        Hists2Plots(modStats)
    if(opts.rootFile):
        Hists2Root(modStats,opts.rootFile)
