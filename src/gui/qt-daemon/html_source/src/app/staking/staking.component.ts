import {Component, NgZone, OnInit, OnDestroy} from '@angular/core';
import {VariablesService} from '../_helpers/services/variables.service';
import {Chart} from 'angular-highcharts';
import {BackendService} from '../_helpers/services/backend.service';
import {ActivatedRoute} from '@angular/router';
import {IntToMoneyPipe} from '../_helpers/pipes/int-to-money.pipe';

@Component({
  selector: 'app-staking',
  templateUrl: './staking.component.html',
  styleUrls: ['./staking.component.scss']
})
export class StakingComponent implements OnInit, OnDestroy {

  parentRouting;
  heightAppEvent;
  refreshStackingEvent;

  periods = [
    {
      title: '1 day',
      active: false
    },
    {
      title: '1 week',
      active: false
    },
    {
      title: '1 month',
      active: false
    },
    {
      title: '1 year',
      active: false
    },
    {
      title: 'All',
      active: true
    }
  ];

  selectedDate = {
    date: null,
    amount: null
  };

  originalData = [];

  chart: Chart;

  total = 0;
  pending = {
    list: [],
    total: 0
  };

  constructor(
    private route: ActivatedRoute,
    private variablesService: VariablesService,
    private backend: BackendService,
    private ngZone: NgZone,
    private intToMoneyPipe: IntToMoneyPipe
  ) {
  }


  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(() => {
      this.getMiningHistory();
    });
    this.heightAppEvent = this.variablesService.getHeightAppEvent.subscribe((newHeight: number) => {
      if (this.pending.total) {
        const pendingCount = this.pending.list.length;
        for (let i = pendingCount - 1; i >= 0; i--) {
          if (newHeight - this.pending.list[i].h >= 10) {
            this.pending.list.splice(i, 1);
          }
        }
        if (pendingCount !== this.pending.list.length) {
          this.pending.total = 0;
          for (let i = 0; i < this.pending.list.length; i++) {
            this.pending.total += this.pending.list[i].a;
          }
        }
      }
    });
    this.refreshStackingEvent = this.variablesService.getRefreshStackingEvent.subscribe((wallet_id: number) => {
      if (this.variablesService.currentWallet.wallet_id === wallet_id) {
        this.getMiningHistory();
      }
    });
  }


  drawChart(data) {
    this.chart = new Chart({
      title: {text: ''},
      credits: {enabled: false},
      exporting: {enabled: false},
      legend: {enabled: false},
      chart: {
        type: 'line',
        backgroundColor: 'transparent',
        height: null,
        zoomType: null
      },

      yAxis: {
        min: 0,
        tickAmount: 5,
        title: {
          text: ''
        },
        gridLineColor: '#2b3644',
        gridLineWidth: 2,
        lineColor: '#2b3644',
        lineWidth: 2,
        tickWidth: 2,
        tickLength: 120,
        tickColor: '#2b3644',
        labels: {
          y: -8,
          align: 'left',
          x: -120,
          style: {
            'color': '#e0e0e0',
            'fontSize': '13px'
          },
          format: '{value} ' + this.variablesService.defaultCurrency
        },
        showLastLabel: false,
      },

      xAxis: {
        type: 'datetime',
        gridLineColor: '#2b3644',
        lineColor: '#2b3644',
        lineWidth: 2,
        tickWidth: 2,
        tickLength: 10,
        tickColor: '#2b3644',
        labels: {
          style: {
            'color': '#e0e0e0',
            'fontSize': '13px'
          }
        },
        minPadding: 0,
        maxPadding: 0,
        minRange: 86400000,
        // tickInterval: 86400000,
        minTickInterval: 3600000,
      },

      tooltip: {
        enabled: false
      },

      plotOptions: {
        area: {
          fillColor: {
            linearGradient: {
              x1: 0,
              y1: 0,
              x2: 0,
              y2: 1
            },
            stops: [
              [0, 'rgba(124,181,236,0.2)'],
              [1, 'rgba(124,181,236,0)']
            ]
          },
          marker: {
            enabled: false,
            radius: 2
          },
          lineWidth: 2,
          threshold: null
        },

        series: {
          point: {
            events: {
              mouseOver: (obj) => {
                this.selectedDate.date = obj.target['x'];
                this.selectedDate.amount = obj.target['y'];
              }
            }
          },
          events: {
            mouseOut: () => {
              this.selectedDate.date = null;
              this.selectedDate.amount = null;
            }
          }
        }
      },
      series: [
        {
          type: 'area',
          data: data
        }
      ]
    });
  }


  getMiningHistory() {
    if (this.variablesService.currentWallet.loaded) {
      this.backend.getMiningHistory(this.variablesService.currentWallet.wallet_id, (status, data) => {
        this.total = 0;
        this.pending.list = [];
        this.pending.total = 0;
        this.originalData = [];
        if (data.mined_entries) {
          data.mined_entries.forEach((item, key) => {
            if (item.t.toString().length === 10) {
              data.mined_entries[key].t = (new Date(item.t * 1000)).setUTCMilliseconds(0);
            }
          });
          data.mined_entries.forEach((item) => {
            this.total += item.a;
            if (this.variablesService.height_app - item.h < 10) {
              this.pending.list.push(item);
              this.pending.total += item.a;
            }
            this.originalData.push([parseInt(item.t, 10), parseFloat(this.intToMoneyPipe.transform(item.a))]);
          });
          this.originalData = this.originalData.sort(function (a, b) {
            return a[0] - b[0];
          });
        }
        this.ngZone.run(() => {
          this.drawChart(JSON.parse(JSON.stringify(this.originalData)));
        });
      });
    }

  }

  changePeriod(period) {
    this.periods.forEach((p) => {
      p.active = false;
    });
    period.active = true;

    const d = new Date();
    let min = null;
    const newData = [];

    if (period.title === '1 day') {
      this.originalData.forEach((item) => {
        const time = (new Date(item[0])).setUTCMinutes(0, 0, 0);
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] += item[1];
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 1, 0, 0, 0, 0);
    } else if (period.title === '1 week') {
      this.originalData.forEach((item) => {
        const time = (new Date(item[0])).setUTCHours(0, 0, 0, 0);
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] += item[1];
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 7, 0, 0, 0, 0);
    } else if (period.title === '1 month') {
      this.originalData.forEach((item) => {
        const time = (new Date(item[0])).setUTCHours(0, 0, 0, 0);
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] += item[1];
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth() - 1, d.getDate(), 0, 0, 0, 0);
    } else if (period.title === '1 year') {
      this.originalData.forEach((item) => {
        const time = (new Date(item[0])).setUTCHours(0, 0, 0, 0);
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] += item[1];
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear() - 1, d.getMonth(), d.getDate(), 0, 0, 0, 0);
    } else {
      this.chart.ref.series[0].setData(this.originalData, true);
    }

    this.chart.ref.xAxis[0].setExtremes(min, null);
  }


  ngOnDestroy() {
    this.parentRouting.unsubscribe();
    this.heightAppEvent.unsubscribe();
    this.refreshStackingEvent.unsubscribe();
  }

}
