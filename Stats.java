/*
 * Modifications by: Suhaskrishna Gopalkrishna
 */
/*
 * RUBiS
 * Copyright (C) 2002, 2003, 2004 French National Institute For Research In Computer
 * Science And Control (INRIA).
 * Contact: jmob@objectweb.org
 * 
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or any later
 * version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * Initial developer(s): Emmanuel Cecchet, Julie Marguerite
 * Contributor(s): 
 */
 package edu.rice.rubis.client;
 import java.io.BufferedWriter;
 import java.io.File;
 import java.io.FileWriter;
 import java.io.IOException;
 import java.net.*;
/**
 * This class provides thread-safe statistics. Each statistic entry is composed as follow:
 * <pre>
 * count     : statistic counter
 * error     : statistic error counter
 * minTime   : minimum time for this entry (automatically computed)
 * maxTime   : maximum time for this entry (automatically computed)
 * totalTime : total time for this entry
 * </pre>
 *
 * @author <a href="mailto:cecchet@rice.edu">Emmanuel Cecchet</a> and <a href="mailto:julie.marguerite@inrialpes.fr">Julie Marguerite</a>
 * @version 1.0
 */

/**
 * Suhaskrishna Gopalkrishna
 * Changes: Modified Stats class constructor to read response time SLA threshold, and the IP of the server to be notified under violation.
 * Modified updateTime() function to log the response times of each request to "/tmp/stat.txt file".
 * Introduced checkSLA() function to monitor the response times of the requests and notify the server in case of SLA violations.
 */

public class Stats
{
  private int nbOfStats;
  private int count[];
  private int error[];
  private long minTime[];
  private long maxTime[];
  private long totalTime[];
  private int  nbSessions;   // Number of sessions succesfully ended
  private long sessionsTime; // Sessions total duration
  private long SLAThreshold; //Response Time SLA threshold to be configured
  private String SLASendIP;//IP of the server to be notified of the SLA violation
  

  /**
   * Creates a new <code>Stats</code> instance.
   * The entries are reset to 0.
   *
   * @param NbOfStats number of entries to create
   */
  public Stats(int NbOfStats, long threshold, String IP)
  {
    nbOfStats = NbOfStats;
    count = new int[nbOfStats];
    error = new int[nbOfStats];
    minTime = new long[nbOfStats];
    maxTime = new long[nbOfStats];
    totalTime = new long[nbOfStats];
    SLAThreshold = threshold;
    SLASendIP = IP;
    reset();
  }


  /**
   * Resets all entries to 0
   */
  public synchronized void reset()
  {
    int i;

    for (i = 0 ; i < nbOfStats ; i++)
    {
      count[i] = 0;
      error[i] = 0;
      minTime[i] = Long.MAX_VALUE;
      maxTime[i] = 0;
      totalTime[i] = 0;
    }
    nbSessions = 0;
    sessionsTime = 0;
  }

  /**
   * Add a session duration to the total sessions duration and
   * increase the number of succesfully ended sessions.
   *
   * @param time duration of the session
   */
  public synchronized void addSessionTime(long time)
  {
    nbSessions++;
    if (time < 0)
    {
      System.err.println("Negative time received in Stats.addSessionTime("+time+")<br>\n");
      return ;
    }
    sessionsTime = sessionsTime + time;
  }

 /**
   * Increment the number of succesfully ended sessions.
   */
  public synchronized void addSession()
  {
    nbSessions++;
  }


  /**
   * Increment an entry count by one.
   *
   * @param index index of the entry
   */
  public synchronized void incrementCount(int index)
  {
    count[index]++;
  }


  /**
   * Increment an entry error by one.
   *
   * @param index index of the entry
   */
  public synchronized void incrementError(int index)
  {
    error[index]++;
  }


  /**
   * Add a new time sample for this entry. <code>time</code> is added to total time
   * and both minTime and maxTime are updated if needed.
   *
   * @param index index of the entry
   * @param time time to add to this entry
   */
  public synchronized void updateTime(int index, long time)
  {
    if (time < 0)
    {
      System.err.println("Negative time received in Stats.updateTime("+time+")<br>\n");
      return ;
    }
    checkSLA(time);
    String str = String.valueOf(time);
    str = str + ",";
    try
    {
      File file = new File("/tmp/stat.txt");
      // if file doesnt exists, then create it
      if (!file.exists())
      {
        file.createNewFile();
      }
      FileWriter fw = new FileWriter(file, true);
      BufferedWriter bw = new BufferedWriter(fw);
      bw.write(str);
      bw.close();
      fw.close();
    }
    catch(IOException e)
    {
      e.printStackTrace();
    }
    totalTime[index] += time;
    if (time > maxTime[index])
      maxTime[index] = time;
    if (time < minTime[index])
      minTime[index] = time;
  }


  public synchronized void checkSLA(long time)
  {
    if(time > SLAThreshold)
    {
        byte[] buffer = {10};
        try
        {
          InetAddress address = InetAddress.getByName(SLASendIP);
          DatagramPacket packet = new DatagramPacket(buffer, buffer.length, address, 7102);
          DatagramSocket datagramSocket = new DatagramSocket();
          datagramSocket.send(packet);
          System.out.println("SLA Violation...");
        }
        catch(IOException e)
        {
          e.printStackTrace();
        }
    }
  }
  /**
   * Get current count of an entry
   *
   * @param index index of the entry
   *
   * @return entry count value
   */
  public synchronized int getCount(int index)
  {
    return count[index];
  }


  /**
   * Get current error count of an entry
   *
   * @param index index of the entry
   *
   * @return entry error value
   */
  public synchronized int getError(int index)
  {
    return error[index];
  }


  /**
   * Get the minimum time of an entry
   *
   * @param index index of the entry
   *
   * @return entry minimum time
   */
  public synchronized long getMinTime(int index)
  {
    return minTime[index];
  }


  /**
   * Get the maximum time of an entry
   *
   * @param index index of the entry
   *
   * @return entry maximum time
   */
  public synchronized long getMaxTime(int index)
  {
    return maxTime[index];
  }


  /**
   * Get the total time of an entry
   *
   * @param index index of the entry
   *
   * @return entry total time
   */
  public synchronized long getTotalTime(int index)
  {
    return totalTime[index];
  }


  /**
   * Get the total number of entries that are collected
   *
   * @return total number of entries
   */
  public int getNbOfStats()
  {
    return nbOfStats;
  }


  /**
   * Adds the entries of another Stats object to this one.
   *
   * @param anotherStat stat to merge with current stat
   */
  public synchronized void merge(Stats anotherStat)
  {
    if (this == anotherStat)
    {
      System.out.println("You cannot merge a stats with itself");
      return;
    }
    if (nbOfStats != anotherStat.getNbOfStats())
    {
      System.out.println("Cannot merge stats of differents sizes.");
      return;
    }
    for (int i = 0 ; i < nbOfStats ; i++)
    {
      count[i] += anotherStat.getCount(i);
      error[i] += anotherStat.getError(i);
      if (minTime[i] > anotherStat.getMinTime(i))
        minTime[i] = anotherStat.getMinTime(i);
      if (maxTime[i] < anotherStat.getMaxTime(i))
        maxTime[i] = anotherStat.getMaxTime(i);
      totalTime[i] += anotherStat.getTotalTime(i);
    }
    nbSessions   += anotherStat.nbSessions;
    sessionsTime += anotherStat.sessionsTime;
  }


  /**
   * Display an HTML table containing the stats for each state.
   * Also compute the totals and average throughput
   *
   * @param title table title
   * @param sessionTime total time for this session
   * @param exclude0Stat true if you want to exclude the stat with a 0 value from the output
   */
  public void display_stats(String title, long sessionTime, boolean exclude0Stat)
  {
    int counts = 0;
    int errors = 0;
    long time = 0;

    System.out.println("<br><h3>"+title+" statistics</h3><p>");
    System.out.println("<TABLE BORDER=1>");
    System.out.println("<THEAD><TR><TH>State name<TH>% of total<TH>Count<TH>Errors<TH>Minimum Time<TH>Maximum Time<TH>Average Time<TBODY>");
    // Display stat for each state
    for (int i = 0 ; i < getNbOfStats() ; i++)
    {
      counts += count[i];
      errors += error[i];
      time += totalTime[i];
    }

    for (int i = 0 ; i < getNbOfStats() ; i++)
    {
      if ((exclude0Stat && count[i] != 0) || (!exclude0Stat))
      {
        System.out.print("<TR><TD><div align=left>"+TransitionTable.getStateName(i)+"</div><TD><div align=right>");
        if ((counts > 0) && (count[i] > 0))
          System.out.print(100*count[i]/counts+" %");
        else
          System.out.print("0 %");
        System.out.print("</div><TD><div align=right>"+count[i]+"</div><TD><div align=right>");
        if (error[i] > 0)
          System.out.print("<B>"+error[i]+"</B>");
        else
          System.out.print(error[i]);
        System.out.print("</div><TD><div align=right>");
        if (minTime[i] != Long.MAX_VALUE)
          System.out.print(minTime[i]);
        else
          System.out.print("0");
        System.out.print(" ms</div><TD><div align=right>"+maxTime[i]+" ms</div><TD><div align=right>");
        if (count[i] != 0)
          System.out.println(totalTime[i]/count[i]+" ms</div>");
        else
           System.out.println("0 ms</div>");
      }
    }

    // Display total   
    if (counts > 0)
    {
      System.out.print("<TR><TD><div align=left><B>Total</B></div><TD><div align=right><B>100 %</B></div><TD><div align=right><B>"+counts+
                       "</B></div><TD><div align=right><B>"+errors+ "</B></div><TD><div align=center>-</div><TD><div align=center>-</div><TD><div align=right><B>");
      counts += errors;
      System.out.println(time/counts+" ms</B></div>");
      // Display stats about sessions
      System.out.println("<TR><TD><div align=left><B>Average throughput</div></B><TD colspan=6><div align=center><B>"+1000*counts/sessionTime+" req/s</B></div>");
      System.out.println("<TR><TD><div align=left>Completed sessions</div><TD colspan=6><div align=left>"+nbSessions+"</div>");
      System.out.println("<TR><TD><div align=left>Total time</div><TD colspan=6><div align=left>"+sessionsTime/1000L+" seconds</div>");
      System.out.print("<TR><TD><div align=left><B>Average session time</div></B><TD colspan=6><div align=left><B>");
      if (nbSessions > 0)
        System.out.print(sessionsTime/(long)nbSessions/1000L+" seconds");
      else
        System.out.print("0 second");
      System.out.println("</B></div>");
    }
    System.out.println("</TABLE><p>");
  }


}
