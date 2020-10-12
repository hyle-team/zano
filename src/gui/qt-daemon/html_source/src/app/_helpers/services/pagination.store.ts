import {Injectable} from '@angular/core';
import {Observable, BehaviorSubject} from 'rxjs';
import {VariablesService} from './variables.service';
import * as _ from 'lodash';

export interface Pages {
   page: number;
  offset: number;
}
@Injectable({
  providedIn: 'root'
})
export class PaginationStore {
  constructor(
    private variablesService: VariablesService
  ) {
  }
  private subject = new BehaviorSubject<Pages[]>(null);
  pages$: Observable<Pages[]> = this.subject.asObservable();

  isForward(pages, currentPage) {
    const max = _.maxBy(pages, 'page');
    return !max || max.page < currentPage || max.page === currentPage;
  }
  setPage(pageNumber: number, offset: number) {
    const pages = this.subject.getValue();
    const current = (this.variablesService.currentWallet.currentPage);
    const isForward = this.isForward(pages, current);
    let newPages: Pages[] = [];
    if (pages && pages.length) {
      newPages = pages.slice(0);
    }
    isForward ? newPages.push({page: pageNumber, offset}) : newPages.pop();
    this.subject.next(newPages);
  }

  get value() {
    return this.subject.value;
  }

}
