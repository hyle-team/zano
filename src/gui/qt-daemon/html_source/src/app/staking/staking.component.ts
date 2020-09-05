import {Component, NgZone, OnInit, OnDestroy} from '@angular/core';
import {VariablesService} from '../_helpers/services/variables.service';
import {Chart} from 'angular-highcharts';
import {BackendService} from '../_helpers/services/backend.service';
import {ActivatedRoute} from '@angular/router';
import {IntToMoneyPipe} from '../_helpers/pipes/int-to-money.pipe';
import {TranslateService} from '@ngx-translate/core';
import {BigNumber} from 'bignumber.js';

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
      title: this.translate.instant('STAKING.PERIOD.WEEK1'),
      key: '1 week',
      active: false
    },
    {
      title: this.translate.instant('STAKING.PERIOD.WEEK2'),
      key: '2 week',
      active: false
    },
    {
      title: this.translate.instant('STAKING.PERIOD.MONTH1'),
      key: '1 month',
      active: false
    },
    {
      title: this.translate.instant('STAKING.PERIOD.MONTH3'),
      key: '3 month',
      active: false
    },
    {
      title: this.translate.instant('STAKING.PERIOD.MONTH6'),
      key: '6 month',
      active: false
    },
    {
      title: this.translate.instant('STAKING.PERIOD.YEAR'),
      key: '1 year',
      active: false
    },
    {
      title: this.translate.instant('STAKING.PERIOD.ALL'),
      key: 'All',
      active: true
    }
  ];

  groups = [
    {
      title: this.translate.instant('STAKING.GROUP.DAY'),
      key: 'day',
      active: true
    },
    {
      title: this.translate.instant('STAKING.GROUP.WEEK'),
      key: 'week',
      active: false
    },
    {
      title: this.translate.instant('STAKING.GROUP.MONTH'),
      key: 'month',
      active: false
    }
  ];

  selectedDate = {
    date: null,
    amount: null
  };

  originalData = [];

  chart: Chart;

  total = new BigNumber(0);
  pending = {
    list: [],
    total: new BigNumber(0)
  };

  constructor(
    private route: ActivatedRoute,
    public variablesService: VariablesService,
    private backend: BackendService,
    private ngZone: NgZone,
    private intToMoneyPipe: IntToMoneyPipe,
    private translate: TranslateService
  ) {
  }

  static makeGroupTime(key, date) {
    if (key === 'day') {
      return date.setHours(0, 0, 0, 0);
    } else if (key === 'week') {
      return new Date(date.setDate(date.getDate() - date.getDay())).setHours(0, 0, 0, 0);
    } else {
      return new Date(date.setDate(1)).setHours(0, 0, 0, 0);
    }
  }

  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(() => {
      this.getMiningHistory();
    });
    this.heightAppEvent = this.variablesService.getHeightAppEvent.subscribe((newHeight: number) => {
      if (!this.pending.total.isZero()) {
        const pendingCount = this.pending.list.length;
        for (let i = pendingCount - 1; i >= 0; i--) {
          if (newHeight - this.pending.list[i].h >= 10) {
            this.pending.list.splice(i, 1);
          }
        }
        if (pendingCount !== this.pending.list.length) {
          this.pending.total = new BigNumber(0);
          for (let i = 0; i < this.pending.list.length; i++) {
            this.pending.total = this.pending.total.plus(this.pending.list[i].a);
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
        zoomType: null,
        events: {
          load: () => {
            this.changePeriod();
          }
        }
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
        this.total = new BigNumber(0);
        this.pending.list = [];
        this.pending.total = new BigNumber(0);
        this.originalData = [];
        if (data.mined_entries) {
          data.mined_entries.forEach((item, key) => {
            if (item.t.toString().length === 10) {
              data.mined_entries[key].t = (new Date(item.t * 1000)).setUTCMilliseconds(0);
            }
          });
          data.mined_entries.forEach((item) => {
            this.total = this.total.plus(item.a);
            if (this.variablesService.height_app - item.h < 10) {
              this.pending.list.push(item);
              this.pending.total = this.pending.total.plus(item.a);
            }
            this.originalData.push([parseInt(item.t, 10), parseFloat(this.intToMoneyPipe.transform(item.a))]);
          });
          this.originalData = this.originalData.sort(function (a, b) {
            return a[0] - b[0];
          });
        }
        this.ngZone.run(() => {
          this.drawChart([]);
        });
      });
    }
  }

  changePeriod(period?) {
    if (period) {
      this.periods.forEach((p) => {
        p.active = false;
      });
      period.active = true;
    } else {
      period = this.periods.find((p) => p.active);
    }

    const d = new Date();
    let min = null;
    const newData = [];

    const group = this.groups.find((g) => g.active);

    if (period.key === '1 week') {
      this.originalData.forEach((item) => {
        const time = StakingComponent.makeGroupTime(group.key, new Date(item[0]));
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] = new BigNumber(find[1]).plus(item[1]).toNumber();
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 7, 0, 0, 0, 0);
    } else if (period.key === '2 week') {
      this.originalData.forEach((item) => {
        const time = StakingComponent.makeGroupTime(group.key, new Date(item[0]));
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] = new BigNumber(find[1]).plus(item[1]).toNumber();
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 14, 0, 0, 0, 0);
    } else if (period.key === '1 month') {
      this.originalData.forEach((item) => {
        const time = StakingComponent.makeGroupTime(group.key, new Date(item[0]));
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] = new BigNumber(find[1]).plus(item[1]).toNumber();
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth() - 1, d.getDate(), 0, 0, 0, 0);
    } else if (period.key === '3 month') {
      this.originalData.forEach((item) => {
        const time = StakingComponent.makeGroupTime(group.key, new Date(item[0]));
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] = new BigNumber(find[1]).plus(item[1]).toNumber();
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth() - 3, d.getDate(), 0, 0, 0, 0);
    } else if (period.key === '6 month') {
      this.originalData.forEach((item) => {
        const time = StakingComponent.makeGroupTime(group.key, new Date(item[0]));
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] = new BigNumber(find[1]).plus(item[1]).toNumber();
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear(), d.getMonth() - 6, d.getDate(), 0, 0, 0, 0);
    } else if (period.key === '1 year') {
      this.originalData.forEach((item) => {
        const time = StakingComponent.makeGroupTime(group.key, new Date(item[0]));
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] = new BigNumber(find[1]).plus(item[1]).toNumber();
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
      min = Date.UTC(d.getFullYear() - 1, d.getMonth(), d.getDate(), 0, 0, 0, 0);
    } else {
      this.originalData.forEach((item) => {
        const time = StakingComponent.makeGroupTime(group.key, new Date(item[0]));
        const find = newData.find(itemNew => itemNew[0] === time);
        if (find) {
          find[1] = new BigNumber(find[1]).plus(item[1]).toNumber();
        } else {
          newData.push([time, item[1]]);
        }
      });
      this.chart.ref.series[0].setData(newData, true);
    }

    this.chart.ref.xAxis[0].setExtremes(min, null);
  }

  changeGroup(group) {
    this.groups.forEach((g) => {
      g.active = false;
    });
    group.active = true;
    this.changePeriod();
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
    this.heightAppEvent.unsubscribe();
    this.refreshStackingEvent.unsubscribe();
  }

}
