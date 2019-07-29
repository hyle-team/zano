import {Component, OnInit, OnDestroy, AfterViewChecked, ViewChild, ElementRef} from '@angular/core';
import {VariablesService} from '../_helpers/services/variables.service';
import {ActivatedRoute} from '@angular/router';
import { Transaction } from '../_helpers/models/transaction.model';

@Component({
  selector: 'app-history',
  templateUrl: './history.component.html',
  styleUrls: ['./history.component.scss']
})
export class HistoryComponent implements OnInit, OnDestroy, AfterViewChecked {
  parentRouting;
  openedDetails = false;
  calculatedWidth = [];
  @ViewChild('head') head: ElementRef;

  constructor(
    private route: ActivatedRoute,
    public variablesService: VariablesService
  ) {}

  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(() => {
      this.openedDetails = false;
    });
  }

  ngAfterViewChecked() {
    this.calculateWidth();
  }

  getHeight(item) {
    if ((this.variablesService.height_app - item.height >= 10 && item.height !== 0) || (item.is_mining === true && item.height === 0)) {
      return 100;
    } else {
      if (item.height === 0 || this.variablesService.height_app - item.height < 0) {
        return 0;
      } else {
        return (this.variablesService.height_app - item.height) * 10;
      }
    }
  }

  openDetails(tx_hash) {
    if (tx_hash === this.openedDetails) {
      this.openedDetails = false;
    } else {
      this.openedDetails = tx_hash;
    }
  }

  calculateWidth() {
    this.calculatedWidth = [];
    this.calculatedWidth.push(this.head.nativeElement.childNodes[0].clientWidth);
    this.calculatedWidth.push(this.head.nativeElement.childNodes[1].clientWidth + this.head.nativeElement.childNodes[2].clientWidth);
    this.calculatedWidth.push(this.head.nativeElement.childNodes[3].clientWidth);
    this.calculatedWidth.push(this.head.nativeElement.childNodes[4].clientWidth);
  }

  time(item: Transaction) {
    const now = new Date().getTime();
<<<<<<< HEAD
    const unlockTime = now + ((item.unlock_time - this.variablesService.height_max) * 60 * 1000);
=======
    const unlockTime = now + ((item.unlock_time - this.variablesService.height_app) * 60 * 1000);
>>>>>>> lock & unlock transaction
    return unlockTime;
  }

  isLocked(item: Transaction) {
    if ((item.unlock_time > 500000000) && (item.unlock_time > new Date().getTime() / 1000)) {
<<<<<<< HEAD
      return true;
    }
    if ((item.unlock_time < 500000000) && (item.unlock_time > this.variablesService.height_max)) {
=======
      console.log(new Date().getTime());
      return true;
    }
    if ((item.unlock_time < 500000000) && (item.unlock_time > this.variablesService.height_app)) {
>>>>>>> lock & unlock transaction
      return true;
    }
    return false;
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
  }

}
